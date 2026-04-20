const std = @import("std");
const mmap = @cImport(@cInclude("memmap.h"));
const allocator = @import("alloc.zig");
const builtin = @import("builtin");

const bitmap_setup = struct {
	bitmap: []allocator.bitmap_t,
	start: usize,
	n_pages: usize
};
const bitmap_region_setup = struct {
	size: usize,
	start: usize,
	end: usize,
	npages: usize
};
const bitmap_offset = struct {bit: usize, word: usize};

pub fn high_mask(comptime T: type, n: u6) T {
    return ~((@as(T, 1) << n) - 1);
}
pub fn low_mask(comptime T: type, n: u7) T {
    return (@as(T, 1) << (n+1)) - 1;
}

pub const PAGE_SIZE = switch(builtin.cpu.arch) {
	.x86_64 => 4096,
	else => @compileError("Unsupported CPU: " ++ builtin.cpu.arch.genericName())
};

pub const bitmap = struct {
	bitmap: []allocator.bitmap_t,
	start_addr: usize,
	n_pages: usize,

	pub fn init_from_memmap(memmap: *mmap.mmap) !bitmap {
		const setup = try setup_bitmap(memmap);
		return .{
			.bitmap = setup.bitmap,
			.start_addr = setup.start,
			.n_pages = setup.n_pages
		};
	}

	inline fn set_entry(self: *@This(), offset: u64, val: u1) !void {
		const bits = @typeInfo(allocator.bitmap_t).int.bits;
		const idx = offset / bits;
		const bit: u6 = @truncate(offset % bits);

		const mask = @as(allocator.bitmap_t, 1) << bit;

		if (val == 1) {
			self.bitmap[idx] |= mask;
		} else {
			self.bitmap[idx] &= ~mask;
		}
	}

	fn addr_to_offset(addr: usize) usize {
		return addr >> 12;
	}

	fn setup_bitmap(memmap: *mmap.mmap) !bitmap_setup {
		const regsetup = try get_necessary_size_for_bitmap(memmap);

		const btmap = try put_bitmap(regsetup.size, memmap);
		try init_bitmap(btmap, memmap, regsetup.start);
		return .{
			.bitmap = btmap,
			.start = regsetup.start,
			.n_pages = regsetup.npages
		};
	}

	fn clear_range(btmap: []allocator.bitmap_t, start: bitmap_offset, end: bitmap_offset) void {
		if (start.word == end.word) {
			const mask = high_mask(allocator.bitmap_t, @truncate(start.bit)) & @as(allocator.bitmap_t, @truncate(low_mask(u128, @truncate(end.bit))));
			btmap[start.word] &= ~mask;
			return;
		}
		btmap[start.word] &= high_mask(allocator.bitmap_t, @truncate(start.bit));
		@memset(btmap[start.word+1 .. end.word], 0);
		btmap[end.word] &= @as(allocator.bitmap_t, @truncate(low_mask(u128, @truncate(end.bit))));
	}

	fn init_bitmap(btmap: []allocator.bitmap_t, memmap: *mmap.mmap, start: usize) !void {
		@memset(btmap, std.math.maxInt(allocator.bitmap_t));
		var entry: ?*mmap.mmap_basic_entry_t = null;
		for(0..memmap.size) |_| {
			entry = mmap.get_next_entry(memmap, entry);
			if(mmap.get_flags(entry.?.addrflags) & mmap.MEM_USABLE != 0) {
				const address = mmap.get_address(entry.?.addrflags);
				if(address != @intFromPtr(btmap.ptr)) {
					const start_offset = calc_offset(address, start);
					const end_offset = calc_offset(address + entry.?.npages*PAGE_SIZE, start);
					clear_range(btmap, start_offset, end_offset);
				} else {
					const start_address = address + std.mem.alignForward(usize, btmap.len, PAGE_SIZE);
					const end_address = address + entry.?.npages*PAGE_SIZE - PAGE_SIZE;
					if(start_address < end_address) {
						const start_offset = calc_offset(start_address, start);
						const end_offset = calc_offset(end_address, start);
						clear_range(btmap, start_offset, end_offset);
					}
				}
			}
		}
	}

	fn get_necessary_size_for_bitmap(memmap: *mmap.mmap) !bitmap_region_setup {
		var first_usable_region: usize = std.math.maxInt(usize);
		var last_usable_region: usize = std.math.maxInt(usize);
		var entry: ?*mmap.mmap_basic_entry_t = null;
		for(0..memmap.size) |i| {
			entry = mmap.get_next_entry(memmap, entry);
			if(mmap.get_flags(entry.?.*.addrflags) & mmap.MEM_USABLE != 0) {
				if(first_usable_region == std.math.maxInt(usize)) {
					first_usable_region = i;
				}
				last_usable_region = i;
			}
		}

		if(first_usable_region == std.math.maxInt(usize)) {
			return error.NO_USABLE_REGIONS;
		}

		const first_entry = mmap.get_nth_entry(memmap, first_usable_region).?;
		const last_entry = mmap.get_nth_entry(memmap, last_usable_region).?;
		const end = mmap.get_address(last_entry.*.addrflags) + last_entry.*.npages*PAGE_SIZE;
		const start = mmap.get_address(first_entry.*.addrflags);

		const all_pages = std.mem.alignForward(usize, end - start, PAGE_SIZE)/PAGE_SIZE;
		const bytesize = (all_pages+7)/8;
		const normsize = (bytesize + @sizeOf(allocator.bitmap_t) - 1) / @sizeOf(allocator.bitmap_t); 

		return .{.size = normsize, .start = start, .end = end, .npages = all_pages};
	}

	fn put_bitmap(size: usize, memmap: *mmap.mmap) ![]allocator.bitmap_t {
		var entry: ?*mmap.mmap_basic_entry_t = null;
		for(0..memmap.size) |_| {
			entry = mmap.get_next_entry(memmap, entry);
			const entry_size = entry.?.npages * PAGE_SIZE;
			if(entry_size >= size and mmap.get_flags(entry.?.addrflags) == mmap.MEM_USABLE) {
				return @as([*]allocator.bitmap_t, @ptrFromInt(mmap.get_address(entry.?.addrflags)))[0..size];
			} 
		}
		return error.NO_REGION_BIG_ENOUGH;
	}

	fn calc_offset(address: usize, start: usize) bitmap_offset {
		const offset = (address - start) / PAGE_SIZE;
		return .{
			.bit = offset % @typeInfo(allocator.bitmap_t).int.bits,
			.word = offset / @typeInfo(allocator.bitmap_t).int.bits
		};
	}
};
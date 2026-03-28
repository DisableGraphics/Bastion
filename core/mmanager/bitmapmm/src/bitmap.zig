const std = @import("std");
const mmap = @cImport(@cInclude("memmap.h"));

const bitmap_t = usize;

const bitmap_setup = struct {
	bitmap: []bitmap_t,
	start: usize
};
const bitmap_region_setup = struct {
	size: usize,
	start: usize,
	end: usize
};
const bitmap_offset = struct {bit: usize, word: usize};

pub fn high_mask(comptime T: type, n: u6) T {
    return ~((@as(T, 1) << n) - 1);
}
pub fn low_mask(comptime T: type, n: u6) T {
    return (@as(T, 1) << n) - 1;
}

pub const PAGE_SIZE = 4096;

pub const bitmap = struct {
	bitmap: []bitmap_t,
	start_addr: usize,
	cache: usize,

	pub fn init_from_memmap(memmap: *mmap.mmap) bitmap {
		const setup = setup_bitmap(memmap);
		return .{
			.bitmap = setup.bitmap,
			.start_addr = setup.start,
			.cache = 0
		};
	}

	inline fn is_entry_active(self: *@This(), offset: u64) !bool {
		const bitmap_displ = offset / @typeInfo(bitmap_t).int.bits;
		const bitmap_mask = offset % @typeInfo(bitmap_t).int.bits; 
		return self.bitmap.?[bitmap_displ] & (@as(bitmap_t, 1) << @truncate(bitmap_mask)) != 0;
	}

	inline fn set_entry(self: @This(), offset: u64, val: u1) !void {
		const bits = @typeInfo(bitmap_t).int.bits;
		const idx = offset / bits;
		const bit: u6 = @truncate(offset % bits);

		const mask = @as(bitmap_t, 1) << bit;

		if (val == 1) {
			self.bitmap.?[idx] |= mask;
		} else {
			self.bitmap.?[idx] &= ~mask;
		}

	}

	fn addr_to_offset(addr: usize) usize {
		return (addr) >> 12;
	}

	pub fn alloc(self: *@This(), npages: usize) !usize {
		var consecutive_entries: usize = 0;
		var start_addr: usize = 0;
		// Find free entries
		for(self.cache..self.bitmap.len) |i| {
			if(!(try is_entry_active(self, i))) {
				// Get start address
				if(start_addr == 0) start_addr = (i << 12);
				consecutive_entries+= 1;
				// If we have enough, allocate them
				if(consecutive_entries == npages) break;
			} else {
				// Reset the search
				consecutive_entries = 0;
				start_addr = 0;
			}
		}
		// If we cannot satisfy the allocation, return null
		if(consecutive_entries < npages) return null; 
		// Set entries to used
		for(0..consecutive_entries) |i| {
			// As i is a page offset, we need to do i << 12 to obtain the physical page address
			const addr = start_addr + (i << 12);
			// Set the entry as allocated
			try set_entry(self, addr_to_offset(addr), 1);
		}
		// Update cache to next free entry
		self.cache = self.cache + consecutive_entries;
		// Return start of physical memory region
		return start_addr + self.start_addr;
	}

	pub fn free(self: *@This(), addr: usize, npages: usize) !void {
		// Set entries as deallocated
		for(0..npages) |i| {
			const physaddr = addr + (i << 12);
			// Set it as free
			try self.set_entry(addr_to_offset(physaddr), 0);
		}
		self.cache = addr / 8;
		return error.NOT_IMPLEMENTED;
	}

	fn setup_bitmap(memmap: *mmap.mmap) bitmap_setup {
		const regsetup = get_necessary_size_for_bitmap(memmap) catch |err| {
			@panic(@errorName(err));
		};
		const btmap = put_bitmap(regsetup.size, memmap) catch |err| {
			@panic(@errorName(err));
		};
		init_bitmap(btmap, memmap, regsetup.start) catch |err| {
			@panic(@errorName(err));
		};
		return .{
			.bitmap = btmap,
			.start = regsetup.start
		};
	}

	fn clear_range(btmap: []bitmap_t, start: bitmap_offset, end: bitmap_offset) void {
		if (start.word == end.word) {
			const mask = high_mask(bitmap_t, @truncate(start.bit)) & low_mask(bitmap_t, @truncate(end.bit));
			btmap[start.word] &= mask;
			return;
		}
		btmap[start.word] &= high_mask(bitmap_t, @truncate(start.bit));
		@memset(btmap[start.word+1 .. end.word], 0);
		btmap[end.word] &= low_mask(bitmap_t, @truncate(end.bit));
	}

	fn init_bitmap(btmap: []bitmap_t, memmap: *mmap.mmap, start: usize) !void {
		@memset(btmap, std.math.maxInt(bitmap_t));
		for(0..memmap.size) |i| {
			const entry = memmap.entries()[i];
			if(mmap.get_flags(entry.addrflags) & mmap.MEM_USABLE != 0) {
				const address = mmap.get_address(entry.addrflags);
				if(address != @intFromPtr(btmap.ptr)) {
					const start_offset = calc_offset(address, start);
					const end_offset = calc_offset(address + entry.npages*PAGE_SIZE, start);
					clear_range(btmap, start_offset, end_offset);
				} else {
					const start_address = address + std.mem.alignForward(usize, btmap.len, PAGE_SIZE);
					const end_address = address + entry.npages*PAGE_SIZE;
					const start_offset = calc_offset(start_address, start);
					const end_offset = calc_offset(end_address, start);
					if(start_address < end_address) {
						clear_range(btmap, start_offset, end_offset);
					}
				}
			}
		}
	}

	fn get_necessary_size_for_bitmap(memmap: *mmap.mmap) !bitmap_region_setup {
		var first_usable_region: usize = std.math.maxInt(usize);
		var last_usable_region: usize = std.math.maxInt(usize);

		for(0..memmap.size) |i| {
			const entry = memmap.entries()[i];
			if(mmap.get_flags(entry.addrflags) & mmap.MEM_USABLE != 0) {
				if(first_usable_region == std.math.maxInt(usize)) {
					first_usable_region = i;
				}
				last_usable_region = i;
			}
		}

		if(first_usable_region == std.math.maxInt(usize)) {
			return error.NO_USABLE_REGIONS;
		}

		const first_entry = memmap.entries()[first_usable_region];
		const last_entry = memmap.entries()[last_usable_region];
		const end = mmap.get_address(last_entry.addrflags) + last_entry.npages*PAGE_SIZE;
		const start = mmap.get_address(first_entry.addrflags);

		const size = ((end - start) + PAGE_SIZE - 1) / PAGE_SIZE;
		return .{.size = (size+7/8), .start = start, .end = end};
	}

	fn put_bitmap(size: usize, memmap: *mmap.mmap) ![]bitmap_t {
		for(0..memmap.size) |i| {
			const entry = memmap.entries()[i];
			const entry_size = entry.npages * PAGE_SIZE;
			if(entry_size >= size and mmap.get_flags(entry.addrflags) == mmap.MEM_USABLE) {
				return @as([*]bitmap_t, @ptrFromInt(mmap.get_address(entry.addrflags)))[0..size];
			} 
		}
		return error.NO_REGION_BIG_ENOUGH;
	}

	fn calc_offset(address: usize, start: usize) bitmap_offset {
		const offset = (address - start) / PAGE_SIZE;
		return .{
			.bit = offset % @sizeOf(bitmap_t),
			.word = offset / @sizeOf(bitmap_t)
		};
	}
};
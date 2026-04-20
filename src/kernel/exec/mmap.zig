const limine = @import("limine");
const mmap = @cImport(@cInclude("memmap.h"));
const kmm = @import("../memory/kmm.zig");
const std = @import("std");
const page = @import("../memory/pagemanager.zig");
const tsk = @import("../scheduler/task.zig");

fn insert_into_size_t_arr(arr: [*]usize, T: type, tp: *anyopaque) usize {
	switch (T) {
		mmap.mmap_basic_entry_t => {
			const a: *mmap.mmap_basic_entry_t = @ptrCast(@alignCast(tp));
			arr[0] = a.addrflags;
			arr[1] = a.npages;
			return 2;
		},
		mmap.mmap_fb_entry_t => {
			const a: *mmap.mmap_fb_entry_t = @ptrCast(@alignCast(tp));
			arr[0] = a.defs.addrflags;
			arr[1] = a.defs.npages;
			arr[2] = (@as(usize, a.width) << 32) | a.height;
			arr[3] = (@as(usize, a.pitch) << 32) | a.bpp;
			arr[4] = (@as(usize, a.red_mask_size) << 32) | a.green_mask_size;
			arr[5] = (@as(usize, a.blue_mask_size) << 32) | @as(usize, a.red_mask_disp) << 24 | @as(usize, a.green_mask_disp) << 16 | @as(usize, a.blue_mask_disp) << 8;
			return 6;
		},
		else => |e| {
			@compileError("Unknown type: " ++ @typeName(e));
		}
	}
}

pub fn create_memory_map(limine_memory_map: *limine.MemoryMapResponse, allocator: *kmm.KernelMemoryManager) !*mmap.mmap {
	var entrycount: usize = 0;
	const ptr: *mmap.mmap = @ptrFromInt((try allocator.alloc_virt(10)) orelse return error.NO_SPACE_LEFT);
	var entr = mmap.get_next_entry(ptr, null);
	for(limine_memory_map.getEntries()) |entry| {
		const start = std.mem.alignForward(usize, entry.base, page.PAGE_SIZE);
		const end = std.mem.alignBackward(usize, entry.base + entry.length, page.PAGE_SIZE);

		if (end <= start) continue;

		const length = (end - start) / page.PAGE_SIZE;
		std.log.info("Entry: {x}-{x} ({}, {})", .{entry.base, entry.base + entry.length, entry.type, length});

		switch(entry.type) {
			.framebuffer => {
				const entr_but_fb: *mmap.mmap_fb_entry_t = @ptrCast(entr);
				entr_but_fb.* = mmap.mmap_fb_entry_t {.defs = .{.addrflags = entry.base | mmap.MEM_FB, .npages = length}};
				std.log.info("Adding {} pages", .{length});
				entrycount += 1;
				entr = mmap.get_next_entry(ptr, entr);
			},
			.acpi_nvs, .acpi_reclaimable, .reserved => {
				entr.* = mmap.mmap_basic_entry_t {.addrflags = entry.base | mmap.MEM_DEVICE, .npages = length};
				std.log.info("Adding {} pages", .{length});
				entrycount += 1;
				entr = mmap.get_next_entry(ptr, entr);
			},
			.usable => {
				// This is more complicated, because I must not make the memory manager be able to see or interact in any way
				// with kernel memory.

				// Therefore, it is necessary to check manually all pages
				var arealength: usize = 0;
				var base: usize = entry.base;
				for(0..length) |adf| {
					const physaddr = entry.base + adf*page.PAGE_SIZE;
					if(try allocator.pfa.is_in_use(physaddr)) {
						if(arealength > 0) {
							entr.* = mmap.mmap_basic_entry_t {.addrflags = base | mmap.MEM_USABLE, .npages = arealength};
							std.log.info("Adding {} pages", .{arealength});
							entrycount += 1;
							arealength = 0;
							entr = mmap.get_next_entry(ptr, entr);
						}
					} else {
						if(arealength == 0) {
							base = physaddr;
						}
						arealength += 1;
					}
				}
				if (arealength > 0) {
					std.log.info("Adding {} pages", .{arealength});
					entr.* = mmap.mmap_basic_entry_t {.addrflags = base | mmap.MEM_USABLE, .npages = arealength};
					entrycount += 1;
					entr = mmap.get_next_entry(ptr, entr);
				}
			},
			else => {
				// Do not make this region visible to the memory manager
			}
		}
	}
	ptr.size = entrycount;
	return ptr;
}

pub fn map_memory_map(task: *tsk.Task, map: *mmap.mmap, pageman: *page.PageManager) !void {
	// Map the memory map so that's visible by the memory manager
	const n_pages_map = ((map.size + 1) * 8 + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
	for(0..n_pages_map) |i| {
		const pageoffset = i*page.PAGE_SIZE;
		const physaddr = try pageman.get_physaddr(pageman.root_table.?, @intFromPtr(map));
		const virtaddr = mmap.MMAP_VIRTADDR + pageoffset;
		try task.addr_space.?.add_mapping_4k(physaddr, virtaddr, .{.us = 1}, .{.us = 1, .rw = 1}, .{.us = 1, .rw = 1}, .{.us = 1, .rw = 1});
	}
	// Map all visible pages in this memory map
	var entry: ?*mmap.mmap_basic_entry_t = null;
	for(0..map.size) |_| {
		entry = mmap.get_next_entry(map, entry);
		std.log.info("Mapping: {x}", .{entry.?.addrflags});
		if(entry.?.addrflags & mmap.MEM_DEVICE != 0) {
			entry.?.addrflags += mmap.OFFSET_MAP;
			std.log.info("Reserved", .{});
			continue;
		} 
		const base = entry.?.addrflags & ~@as(usize, 0xFFF);
		const npages = entry.?.npages;
		std.log.info("Mappeation: {x} ({} pages)", .{base, npages});
		for(0..npages) |n| {
			const offset = page.PAGE_SIZE*n;
			
			const physaddr = base + offset;
			const virtaddr = physaddr + mmap.OFFSET_MAP;
			try task.addr_space.?.add_mapping_4k(
				physaddr,
				virtaddr, 
				.{.us = 1, .rw = 1}, 
				.{.us = 1, .rw = 1}, 
				.{.us = 1, .rw = 1}, 
				.{.us = 1, .rw = 1}
			);
		}
		entry.?.addrflags += mmap.OFFSET_MAP;
	}
	std.log.info("Unmapping map", .{});
	// Now I can unmap the memory map from the kernel
	for(0..n_pages_map) |i| {
		const pageoffset = i*page.PAGE_SIZE;
		const virtaddr = @intFromPtr(map) + pageoffset;
		try pageman.unmap(pageman.root_table.?, virtaddr);
	}
}
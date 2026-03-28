const limine = @import("limine");
const mmap = @cImport(@cInclude("memmap.h"));
const kmm = @import("../memory/kmm.zig");
const std = @import("std");
const page = @import("../memory/pagemanager.zig");
const tsk = @import("../scheduler/task.zig");

pub fn create_memory_map(limine_memory_map: *limine.MemoryMapResponse, allocator: *kmm.KernelMemoryManager) !*mmap.mmap {
	var i: usize = 0;
	const ptr: *mmap.mmap = @ptrFromInt((try allocator.alloc_virt(10)) orelse return error.NO_SPACE_LEFT);
	for(limine_memory_map.getEntries()) |entry| {
		std.log.info("Entry: {x}-{x} ({})", .{entry.base, entry.base + entry.length, entry.type});
		const start = std.mem.alignForward(usize, entry.base, page.PAGE_SIZE);
		const end = std.mem.alignBackward(usize, entry.base + entry.length, page.PAGE_SIZE);

		if (end <= start) continue;

		const length = (end - start) / page.PAGE_SIZE;

		switch(entry.type) {
			.framebuffer => {
				ptr.entries()[i] = mmap.struct_mmap_entry {.addrflags = entry.base | mmap.MEM_FB, .npages = length};
				i += 1;
			},
			.acpi_nvs, .acpi_reclaimable, .reserved => {
				ptr.entries()[i] = mmap.struct_mmap_entry {.addrflags = entry.base | mmap.MEM_DEVICE, .npages = length};
				std.log.info("Reserved: {x}", .{ptr.entries()[i].addrflags});
				i += 1;
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
							ptr.entries()[i] = mmap.struct_mmap_entry {.addrflags = base | mmap.MEM_USABLE, .npages = arealength};
							i += 1;
							arealength = 0;
						}
					} else {
						if(arealength == 0) {
							base = physaddr;
						}
						arealength += 1;
					}
				}
				if (arealength > 0) {
					ptr.entries()[i] = mmap.struct_mmap_entry{
						.addrflags = base | mmap.MEM_USABLE,
						.npages = arealength,
					};
					i += 1;
				}
			},

			else => {
				// Do not make this region visible to the memory manager
			}
		}
	}
	ptr.size = i;
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
	for(0..map.size) |i| {
		const entry = map.entries()[i];
		std.log.info("Mapping: {x}", .{entry.addrflags});
		if(entry.addrflags & mmap.MEM_DEVICE != 0) {
			map.entries()[i].addrflags += mmap.OFFSET_MAP;
			std.log.info("Reserved", .{});
			continue;
		} 
		const base = entry.addrflags & ~@as(usize, 0xFFF);
		const npages = entry.npages;
		std.log.info("{x}", .{base});
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
		map.entries()[i].addrflags += mmap.OFFSET_MAP;
	}
	std.log.info("Unmapping map", .{});
	// Now I can unmap the memory map from the kernel
	for(0..n_pages_map) |i| {
		const pageoffset = i*page.PAGE_SIZE;
		const virtaddr = @intFromPtr(map) + pageoffset;
		try pageman.unmap(pageman.root_table.?, virtaddr);
	}
}
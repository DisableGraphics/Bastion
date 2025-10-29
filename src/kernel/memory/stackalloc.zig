const slab = @import("../memory/slaballoc.zig");
const main = @import("../main.zig");
const kmm = @import("../memory/kmm.zig");
const page = @import("../memory/pagemanager.zig");
const std = @import("std");

pub const KernelStack = [4*page.PAGE_SIZE]u8;

pub const KernelStackAllocator = struct {
	var allocators: []slab.SlabAllocator(KernelStack) = undefined;
	pub fn init(expected_tasks: u64, mp_cores: u64, km: *kmm.KernelMemoryManager) !void {
		const allocators_pages = ((mp_cores * @sizeOf(slab.SlabAllocator(KernelStack))) + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
		allocators = @as([*]slab.SlabAllocator(KernelStack), @ptrFromInt((try km.alloc_virt(allocators_pages)).?))[0..mp_cores];

		// Force at least 64 tasks available per core
		const expected_tasks_per_core = @max(64, (expected_tasks + mp_cores - 1) / mp_cores);
		for(0..mp_cores) |i| {
			const expected_tasks_pages = ((expected_tasks_per_core * @sizeOf(KernelStack)) + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
			const ntasks_expected = expected_tasks_pages * page.PAGE_SIZE;
			const buffer = @as([*]u8, @ptrFromInt((try km.alloc_virt(expected_tasks_pages)).?))[0..ntasks_expected];
			allocators[i] = try slab.SlabAllocator(KernelStack).init(buffer);
			try allocators[i].init_region();
		}

	}

	pub fn alloc() ?*KernelStack {
		const myid = main.mycpuid();
		const buf = allocators[myid].alloc();
		if(buf == null) return null;
		@memset(buf.?, 0);
		return buf;
	}

	pub fn free(ts: *KernelStack) !void {
		const myid = main.mycpuid();
		return allocators[myid].free(ts);
	}
};
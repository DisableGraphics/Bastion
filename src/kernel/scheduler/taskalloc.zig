const slab = @import("../memory/slaballoc.zig");
const tsk = @import("task.zig");
const main = @import("../main.zig");
const kmm = @import("../memory/kmm.zig");
const page = @import("../memory/pagemanager.zig");
const std = @import("std");

pub const TaskAllocator = struct {
	var allocators: []slab.SlabAllocator(tsk.Task) = undefined;
	pub fn init(expected_tasks: u64, mp_cores: u64, km: *kmm.KernelMemoryManager) !void {
		const allocators_pages = ((mp_cores * @sizeOf(slab.SlabAllocator(tsk.Task))) + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
		allocators = @as([*]slab.SlabAllocator(tsk.Task), @ptrFromInt((try km.alloc_virt(allocators_pages)).?))[0..mp_cores];

		// Force at least 64 tasks available per core
		const expected_tasks_per_core = @max(64, (expected_tasks + mp_cores - 1) / mp_cores);
		for(0..mp_cores) |i| {
			const expected_tasks_pages = ((expected_tasks_per_core * @sizeOf(tsk.Task)) + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
			const ntasks_expected = expected_tasks_pages * page.PAGE_SIZE;
			const buffer = @as([*]u8, @ptrFromInt((try km.alloc_virt(expected_tasks_pages)).?))[0..ntasks_expected];
			allocators[i] = try slab.SlabAllocator(tsk.Task).init(buffer);
			try allocators[i].init_region();
		}

	}

	pub fn alloc() ?*tsk.Task {
		const myid = main.mycpuid();
		return allocators[myid].alloc();
	}

	pub fn free(ts: *tsk.Task) !void {
		const myid = main.mycpuid();
		return allocators[myid].free(ts);
	}
};
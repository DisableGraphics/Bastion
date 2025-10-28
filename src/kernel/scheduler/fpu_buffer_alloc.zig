const slab = @import("../memory/slaballoc.zig");
const tsk = @import("task.zig");
const main = @import("../main.zig");
const kmm = @import("../memory/kmm.zig");
const page = @import("../memory/pagemanager.zig");
const std = @import("std");

pub const FPU_SUPPORTED = enum {
	SSE,
	AVX,
	AVX512,
};

const fpu_buffer = [2872]u8;

pub const FPUBufferAllocator = struct {
	var allocators: []slab.SlabAllocator(fpu_buffer) = undefined;
	pub fn init(expected_tasks: u64, mp_cores: u64, km: *kmm.KernelMemoryManager) !void {
		const allocators_pages = ((mp_cores * @sizeOf(slab.SlabAllocator(fpu_buffer))) + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
		allocators = @as([*]slab.SlabAllocator(fpu_buffer), @ptrFromInt((try km.alloc_virt(allocators_pages)).?))[0..mp_cores];

		// Force at least 64 buffers available per core
		const expected_buffers_per_core = @max(64, (expected_tasks + mp_cores - 1) / mp_cores);
		for(0..mp_cores) |i| {
			const expected_buffers_pages = ((expected_buffers_per_core * @sizeOf(fpu_buffer)) + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
			const nbuffers_expected = expected_buffers_pages * page.PAGE_SIZE;
			const buffer = @as([*]u8, @ptrFromInt((try km.alloc_virt(expected_buffers_pages)).?))[0..nbuffers_expected];
			allocators[i] = try slab.SlabAllocator(fpu_buffer).init(buffer);
			try allocators[i].init_region();
		}

	}

	pub fn alloc() ?*fpu_buffer {
		const myid = main.mycpuid();
		return allocators[myid].alloc();
	}

	pub fn free(ts: *fpu_buffer) !void {
		const myid = main.mycpuid();
		return allocators[myid].free(ts);
	}
};
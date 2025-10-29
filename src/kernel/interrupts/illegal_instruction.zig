const handlers = @import("handlers.zig");
const main = @import("../main.zig");
const std = @import("std");
const cid = @import("../arch/x86_64/cpuid.zig");
const schman = @import("../scheduler/manager.zig");
const bufalloc = @import("../scheduler/fpu_buffer_alloc.zig");

pub var supports_avx = false;

pub fn setup_supports_avx() void {
	const res = cid.cpuid(1, 0);
	// Both supports avx and xsave bit
	supports_avx = res.ecx & (1 << 28) != 0 and res.ecx & (1 << 26) != 0;
}

pub fn enable_vector() void {
	// At least sse1 & sse2 are mandatory for x86_64
	enable_sse();
	if(supports_avx) {
		enable_osxsave();
		enable_avx();
	}
}

fn clear_vector() void {
	clear_sse(); // Although it says clear_sse it works for all vector extensions
}
// Clears cr0.ts flag
extern fn clear_sse() callconv(.C) void;
// Enables SSE
extern fn enable_sse() callconv(.C) void;

extern fn enable_osxsave() callconv(.C) void;
extern fn enable_avx() callconv(.C) void;
// Restores SSE context using fxrstor
extern fn restore_sse(ptr: *u8) callconv(.C) void;
extern fn restore_avx(ptr: *u8) callconv(.C) void;

fn restore_vector(ptr: *u8) void {
	if(supports_avx) {
		restore_avx(ptr);
	} else {
		restore_sse(ptr);
	}
}

extern fn save_sse(ptr: *u8) callconv(.C) void;
extern fn save_avx(ptr: *u8) callconv(.C) void;
extern fn disable_sse() callconv(.C) void;

pub fn save_vector(ptr: *u8) void {
	if(supports_avx) {
		save_avx(ptr);
	} else {
		save_sse(ptr);
	}
}

pub fn disable_vector() void {
	disable_sse();
}

pub fn ill_instr(frame: *handlers.IdtFrame) callconv(.Interrupt) void {
	_ = frame;
	// This one is funny: if we have an SSE/AVX/MMX/x87 instruction we need to search whether the current task we're running
	// has a buffer to load from. If it doesn't, we have to allocate a buffer for the task and load from it.
	// If it has a buffer, then just loading it is enough.
	// and also of course to clear cr0.
	const mycpuid = main.mycpuid();
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(mycpuid);
	sch.lock();
	defer sch.unlock();
	var task = sch.current_process.?;
	const has_buffer = task.fpu_buffer != null;
	if(!has_buffer) {
		task.fpu_buffer = bufalloc.FPUBufferAllocator.alloc();
		if(task.fpu_buffer != null) {
			task.cpu_fpu_buffer_created_on = mycpuid;
			@memset(task.fpu_buffer.?, 0);
		}
	}
	if(task.fpu_buffer != null) {
		clear_vector(); // Clear bit ts in cr0 so that no more problems come to us.
		// Load buffer
		restore_vector(&task.fpu_buffer.?.*[0]);
		task.has_used_vector = true;
	} else {
		// we're out of buffers fuc
	}

}
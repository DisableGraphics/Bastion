const slab = @import("../memory/slaballoc.zig");
const tsk = @import("task.zig");
const main = @import("../main.zig");
const kmm = @import("../memory/kmm.zig");
const page = @import("../memory/pagemanager.zig");
const std = @import("std");
const ii = @import("../interrupts/illegal_instruction.zig");
const ma = @import("../memory/multialloc.zig");

pub const fpu_buffer = [2880]u8;
pub const FPUBufferAllocator = ma.MultiAlloc(fpu_buffer, true, 56);

pub fn save_fpu_buffer(task: *tsk.Task) void {
	ii.save_vector(&task.fpu_buffer.?[0]);
	// So that it traps next time and we just load the SSE context
	ii.disable_vector();
	task.has_used_vector = false;
}
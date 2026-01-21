const schman = @import("manager.zig");
const tsk = @import("task.zig");
const std = @import("std");
const ipi = @import("../interrupts/ipi_protocol.zig");

fn atomicAddModulo(counter: *std.atomic.Value(u32), add: u32, modulo: u32) u32 {
    while (true) {
        const old = counter.load(.seq_cst);
        const new = (old + add) % modulo;
        if (counter.cmpxchgWeak(old, new, .seq_cst, .seq_cst) == null) {
            return new;
        }
        // If CAS fails, loop and try again
    }
}

pub const TaskAdder = struct {
	var counter = std.atomic.Value(u32).init(0);
	pub fn add_task(task: *tsk.Task) void {
		// Round-robin
		const dest = atomicAddModulo(&counter, 1, @truncate(schman.SchedulerManager.schedulers.len));
		
		ipi.IPIProtocolHandler.send_ipi(
			dest, 
			ipi.IPIProtocolPayload.init_with_data(
				ipi.IPIProtocolMessageType.TASK_LOAD_BALANCING_RESPONSE, 
				@intFromPtr(task), 0, 0));
	}

	pub fn add_blocked_task(task: *tsk.Task) void {
		// Round-robin
		const dest = atomicAddModulo(&counter, 1, @truncate(schman.SchedulerManager.schedulers.len));
		
		ipi.IPIProtocolHandler.send_ipi(
			dest, 
			ipi.IPIProtocolPayload.init_with_data(
				ipi.IPIProtocolMessageType.TASK_LOAD_BALANCING_RESPONSE_BLOCKED, 
				@intFromPtr(task), 0, 0));
	}
};
const schman = @import("manager.zig");
const sched = @import("scheduler.zig");
const cpuid = @import("../main.zig");
const spin = @import("../sync/spinlock.zig");
const std = @import("std");
const task = @import("task.zig");
const ipi = @import("../interrupts/ipi_protocol.zig");

// Load threshold
const high_load_threshold: comptime_int = @intFromFloat(@as(f64, @floatFromInt(std.math.maxInt(u32) / 10)) * 9);

pub const LoadBalancer = struct {
	pub fn find_task(sch: *sched.Scheduler) ?*task.Task {
		sch.lock();
		defer sch.unlock();
		for(0..sched.queue_len) |i| {
			if(sch.queues[i] != null) {
				const start = sch.queues[i].?;
				var t = start.next;
				var check_once = true;
				while(t.? != start or check_once) {
					if(!t.?.is_pinned and sch.current_process != t) {
						sch.remove_task_from_list(t.?, &sch.queues[i]);
						return t;
					}
					check_once = false;
					t = t.?.next;
				}
			}
		}
		return null;
	}
	pub fn steal_task_async() void {
		const myid = cpuid.mycpuid();
		for(0..schman.SchedulerManager.schedulers.len) |i| {
			if(i != myid) {
				const sch = schman.SchedulerManager.get_scheduler_for_cpu(i);
				const load = sch.get_load();
				if(load > high_load_threshold) {
					ipi.IPIProtocolHandler.send_ipi(@truncate(i), ipi.IPIProtocolPayload.init_with_data(
						ipi.IPIProtocolMessageType.TASK_LOAD_BALANCING_REQUEST,
						myid,
						0,
						0));
				}
			}
		}
	}
};
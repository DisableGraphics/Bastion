const task = @import("task.zig");
const ts = @import("../memory/tss.zig");
const std = @import("std");
const idt = @import("../interrupts/idt.zig");

extern fn switch_task(
	current_task: **task.Task,
	next_task: *task.Task,
	tss: *ts.tss_t
	) callconv(.C) void;
// NOTE: ONE SCHEDULER PER CPU CORE
pub const Scheduler = struct {
	tasks: ?*task.Task,
	current_process: ?*task.Task,
	idle_task: ?*task.Task,
	cpu_tss: *ts.tss_t,
	pub fn init(cpu_tss: *ts.tss_t) Scheduler {
		return .{
			.idle_task = null,
			.current_process = null,
			.tasks = null,
			.cpu_tss = cpu_tss,
		};
	}

	pub fn add_task(self: *Scheduler, tas: *task.Task) void {
		if(self.tasks) |proc| {
			var procn: ?*task.Task = proc.next;
			procn.?.next = tas;
			tas.next = proc;
		} else {
			self.tasks = tas;
			self.tasks.?.next = self.tasks.?;
		}
	}

	pub fn add_idle(self: *Scheduler, tas: *task.Task) void {
		self.idle_task = tas;
		self.current_process = tas;
	}

	// Assumes tasks is a circular linked list (which should be with how processes are allocated)
	fn search_available_task(self: *Scheduler, start_at: *task.Task) ?*task.Task {
		_ = self;
		var tptr = start_at.next.?;
		while(tptr != start_at) : (tptr = tptr.next.?) {
			if(tptr.state == task.TaskStatus.READY) {
				return tptr;
			}
		}
		return null;
	}

	// Needs to have a idle task set up, or it will panic
	fn next_task(self: *Scheduler) *task.Task {
		if(self.current_process) |proc| {
			if(proc == self.idle_task.?) {
				if(self.tasks) |start| {
					// If tasks are available, return task
					if(self.search_available_task(start)) |t| {
						return t;
					}
				}
				return proc;
			} else {
				if(self.search_available_task(proc)) |t| {
					return t;
				}
			}
		}
		return self.idle_task.?;
	}

	pub fn schedule(self: *Scheduler) void {
		idt.disable_interrupts();
		if(self.tasks) |_| {
			if(self.current_process) |_| {
				const t = self.next_task();
				std.log.debug("{x}", .{&t});
				switch_task(
					&self.current_process.?,
					t,
					self.cpu_tss);
			}
			// Hasn't been setup with an idle task
		}
		idt.enable_interrupts();
	}

	pub fn block(self: *Scheduler, reason: task.TaskStatus) void {
		idt.disable_interrupts();
		self.current_process.?.state = reason;
		self.schedule();
	}
};
const task = @import("task.zig");
const ts = @import("../memory/tss.zig");
const std = @import("std");
const idt = @import("../interrupts/idt.zig");

extern fn switch_task(
	current_task: **task.Task,
	next_task: *task.Task,
	tss: *ts.tss_t,
	current_task_deleted: u64,
	) callconv(.C) void;
// NOTE: ONE SCHEDULER PER CPU CORE
pub const Scheduler = struct {
	tasks: ?*task.Task,
	current_process: ?*task.Task,
	idle_task: ?*task.Task,
	cpu_tss: *ts.tss_t,
	finished_tasks: ?*task.Task,
	pub fn init(cpu_tss: *ts.tss_t) Scheduler {
		return .{
			.idle_task = null,
			.current_process = null,
			.tasks = null,
			.finished_tasks = null,
			.cpu_tss = cpu_tss,
		};
	}

	pub fn add_task(self: *Scheduler, tas: *task.Task) void {
		idt.disable_interrupts();
		if(self.tasks) |_| {
			var head = self.tasks.?;
			var last = head.prev.?;
			var data = tas;
			data.next = head;
			data.prev = last;
			head.prev = data;
			last.next = data;
		} else {
			self.tasks = tas;
			self.tasks.?.next = self.tasks;
			self.tasks.?.prev = self.tasks;
		}
		idt.enable_interrupts();
	}

	pub fn add_idle(self: *Scheduler, tas: *task.Task) void {
		idt.disable_interrupts();
		self.idle_task = tas;
		self.current_process = tas;
		idt.enable_interrupts();
	}

	// Assumes tasks is a circular linked list (which should be with how processes are allocated)
	fn search_available_task(self: *Scheduler, start_at: *task.Task) ?*task.Task {
		_ = self;
		var tptr = start_at.next.?;
		var one_advance = true;
		while(tptr != start_at or one_advance) : (tptr = tptr.next.?) {
			if(tptr.state == task.TaskStatus.READY) {
				return tptr;
			}
			one_advance = false;
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
				if(proc.state == task.TaskStatus.FINISHED) {
					if(self.tasks) |start| {
						if(self.search_available_task(start)) |t| {
							return t;
						}
					}
					return self.idle_task.?;
				}
				if(self.search_available_task(proc)) |t| {
					return t;
				}
			}
		}
		return self.idle_task.?;
	}

	fn clear_deleted_tasks(self: *Scheduler) void {
		if(self.tasks) |_| {
			var tptr = self.tasks.?;
			var one_advance = true;
			while(self.tasks != null and (tptr != self.tasks.? or one_advance)) : (tptr = tptr.next.?) {
				if(tptr.state == task.TaskStatus.FINISHED) {
					self.remove(tptr);
				}
				one_advance = false;
			}
		}
	}

	pub fn schedule(self: *Scheduler) void {
		idt.disable_interrupts();
		if(self.tasks) |_| {
			if(self.current_process) |_| {
				self.clear_deleted_tasks();
				const t = self.next_task();
				switch_task(
					&self.current_process.?,
					t,
					self.cpu_tss,
					@intFromBool(self.current_process.?.state == task.TaskStatus.FINISHED));
			}
			// Hasn't been setup with an idle task
		}
		idt.enable_interrupts();
	}

	pub fn block(self: *Scheduler, tsk: *task.Task, reason: task.TaskStatus) void {
		idt.disable_interrupts();
		tsk.state = reason;
		self.schedule();
	}
	pub fn unblock(self: *Scheduler, tsk: *task.Task) void {
		idt.disable_interrupts();
		_ = self;
		tsk.state = task.TaskStatus.READY;
		idt.enable_interrupts();
	}

	fn remove(self: *Scheduler, tsk: *task.Task) void {
		self.remove_task_from_list(tsk);
		if(tsk.deinitfn) |_| {
			tsk.deinitfn.?(tsk, tsk.extra_arg);
		}
	}

	fn remove_task_from_list(self: *Scheduler, tsk: *task.Task) void {
		if(self.tasks) |proc| {
			if(tsk.next.? == tsk and tsk.prev.? == tsk and tsk == proc) {
				self.tasks = null;
			} else {
				if(tsk == proc) {
					self.tasks = tsk.next;
				}
				if (tsk.prev) |prev| {
					prev.next = tsk.next;
				}
				if (tsk.next) |next| {
					next.prev = tsk.prev;
				}
			}
		}
	}
};
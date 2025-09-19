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
	l1_tasks: ?*task.Task,
	l2_tasks: ?*task.Task,
	l3_tasks: ?*task.Task,
	l4_tasks: ?*task.Task,
	current_process: ?*task.Task,
	blocked_tasks: ?*task.Task,
	idle_task: ?*task.Task,
	cleanup_task: ?*task.Task,
	cpu_tss: *ts.tss_t,
	finished_tasks: ?*task.Task,
	pub fn init(cpu_tss: *ts.tss_t) Scheduler {
		return .{
			.idle_task = null,
			.current_process = null,
			.blocked_tasks = null,
			.l1_tasks = null,
			.l2_tasks = null,
			.l3_tasks = null,
			.l4_tasks = null,
			.finished_tasks = null,
			.cleanup_task = null,
			.cpu_tss = cpu_tss,
		};
	}
	// Adds a new task
	pub fn add_task(self: *Scheduler, tas: *task.Task) void {
		idt.disable_interrupts();
		self.add_task_to_list(tas, &self.l1_tasks);
		idt.enable_interrupts();
	}
	// adds an idle task & sets up current process
	// from now on we can schedule tasks
	pub fn add_idle(self: *Scheduler, tas: *task.Task) void {
		idt.disable_interrupts();
		self.idle_task = tas;
		self.current_process = tas;
		idt.enable_interrupts();
	}
	// adds a cleanup task
	pub fn add_cleanup(self: *Scheduler, tas: *task.Task) void {
		idt.disable_interrupts();
		self.cleanup_task = tas;
		self.cleanup_task.?.state = task.TaskStatus.SLEEPING;
		idt.enable_interrupts();
	}

	// Assumes tasks is a circular linked list (which should be with how processes are allocated)
	// Also O(1)
	fn search_available_task(self: *Scheduler, start_at: *task.Task) ?*task.Task {
		_ = self;
		return start_at.next.?;
	}

	// Needs to have a idle task set up, or it will panic
	fn next_task(self: *Scheduler) *task.Task {
		// Cleanup task has the highest priority. Why? because memory is limited. We need to free the dead process as soon as possible.
		if(self.cleanup_task) |t| {
			if(t.state == task.TaskStatus.READY) return t;
		}
		// Basically:
		//    - If current process is a special task (either idle or cleanup) search for another task. If there isn't one, just return the idle
		//    - If current process is a normal task, just check if hasn't finished. If it is, search for another task (if no tasks available return the idle)
		//    - If it hasn't finished, return the next task.
		if(self.current_process) |proc| {
			if(proc == self.idle_task.? or proc == self.cleanup_task.?) {
				if(self.l1_tasks) |start| {
					// If tasks are available, return task
					if(self.search_available_task(start)) |t| {
						return t;
					}
				}
				return self.idle_task.?;
			} else {
				if(proc.state == task.TaskStatus.FINISHED) {
					if(self.l1_tasks) |start| {
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

	pub fn clear_deleted_tasks(self: *Scheduler) void {
		idt.disable_interrupts();
		if(self.blocked_tasks) |_| {
			var tptr = self.blocked_tasks.?;
			var one_advance = true;
			while(self.blocked_tasks != null and (tptr != self.blocked_tasks.? or one_advance)) : (tptr = tptr.next.?) {
				if(tptr.state == task.TaskStatus.FINISHED) {
					self.remove(tptr);
				}
				one_advance = false;
			}
		}
		idt.enable_interrupts();
	}

	pub fn schedule(self: *Scheduler) void {
		idt.disable_interrupts();
		// If current process has been assigned (by setting up an idle task)
		// then we just search for the next one.
		if(self.current_process != null) {
			const t = self.next_task();
				switch_task(
					&self.current_process.?,
					t,
					self.cpu_tss,
					@intFromBool(self.current_process.?.state == task.TaskStatus.FINISHED));
		}
		idt.enable_interrupts();
	}

	pub fn block(self: *Scheduler, tsk: *task.Task, reason: task.TaskStatus) void {
		idt.disable_interrupts();
		// If it was unlocked, we need to move it to the correct list.
		// If it wasn't unlocked, this would be a no-op.
		const was_unlocked = tsk.state == task.TaskStatus.READY;
		tsk.state = reason;
		if(tsk != self.idle_task.? and tsk != self.cleanup_task.? and was_unlocked) {
			self.remove_task_from_list(tsk, &self.l1_tasks);
			self.add_task_to_list(tsk, &self.blocked_tasks);
		}
		self.schedule();
	}
	pub fn unblock(self: *Scheduler, tsk: *task.Task) void {
		idt.disable_interrupts();
		// If it was locked, we need to move it to the correct list.
		// If it wasn't locked, this would be a no-op.
		const was_locked = tsk.state != task.TaskStatus.READY;
		tsk.state = task.TaskStatus.READY;
		if(tsk != self.idle_task.? and tsk != self.cleanup_task.? and was_locked) {
			// Move the task from
			self.remove_task_from_list(tsk, &self.blocked_tasks);
			self.add_task_to_list(tsk, &self.l1_tasks);
		}
		if(self.current_process.? == self.idle_task.?) {
			// Preempt the idle task if it was the current running process
			self.schedule();
		}
		idt.enable_interrupts();
	}
	pub fn exit(self: *Scheduler, tsk: *task.Task) void {
		// Unblock the cleanup task to free the deleted task's resources
		if(self.cleanup_task != null) {
			self.unblock(self.cleanup_task.?);
		}
		// Mark the task as finished
		self.block(tsk, task.TaskStatus.FINISHED);
	}

	fn remove(self: *Scheduler, tsk: *task.Task) void {
		// As when finalizing tasks we call block(), the task is in the
		// blocked_tasks list
		self.remove_task_from_list(tsk, &self.blocked_tasks);
		if(tsk.deinitfn) |_| {
			tsk.deinitfn.?(tsk, tsk.extra_arg);
		}
	}

	fn add_task_to_list(self: *Scheduler, tas: *task.Task, list: *?*task.Task) void {
		_ = self;
		// Two cases: there is a node and no node
		if(list.*) |_| {
			// Add at the end the node
			var head = list.*.?;
			var last = head.prev.?;
			var data = tas;
			data.next = head;
			data.prev = last;
			head.prev = data;
			last.next = data;
		} else {
			// Basically:
			// - add the node
			// - make the node point to itself
			list.* = tas;
			list.*.?.next = list.*;
			list.*.?.prev = list.*;
		}
	}

	fn remove_task_from_list(self: *Scheduler, tsk: *task.Task, list: *?*task.Task) void {
		_ = self;
		if(list.*) |proc| {
			if(tsk.next.? == tsk and tsk.prev.? == tsk and tsk == proc) { // Only one element in the list
				list.* = null;
			} else { // More than one element in the list
				if(tsk == proc) {
					list.* = tsk.next;
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
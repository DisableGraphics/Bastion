const task = @import("task.zig");
const ts = @import("../memory/tss.zig");
const std = @import("std");
const idt = @import("../interrupts/idt.zig");
const queue_len = 4;

extern fn switch_task(
	current_task: **task.Task,
	next_task: *task.Task,
	tss: *ts.tss_t,
	current_task_deleted: u64,
	) callconv(.C) void;
// NOTE: ONE SCHEDULER PER CPU CORE
// NOTE2: DO NOT LOG ANYTHING EXCEPT IN NON-INTERRUPT CONTEXTS.
pub const Scheduler = struct {
	queues: [queue_len]?*task.Task,
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
			.queues = [_]?*task.Task{null} ** queue_len,
			.finished_tasks = null,
			.cleanup_task = null,
			.cpu_tss = cpu_tss,
		};
	}

	pub fn lock(self: *Scheduler) void {
		_ = self;
		idt.disable_interrupts();
	}
	pub fn unlock(self: *Scheduler) void {
		_ = self;
		idt.enable_interrupts();
	}
	// Adds a new task
	pub fn add_task(self: *Scheduler, tas: *task.Task) void {
		self.lock();
		self.add_task_to_list(tas, &self.queues[0]);
		self.unlock();
	}
	// adds an idle task & sets up current process
	// from now on we can schedule tasks
	pub fn add_idle(self: *Scheduler, tas: *task.Task) void {
		self.lock();
		self.idle_task = tas;
		self.current_process = tas;
		self.unlock();
	}
	// adds a cleanup task
	pub fn add_cleanup(self: *Scheduler, tas: *task.Task) void {
		self.lock();
		self.cleanup_task = tas;
		self.cleanup_task.?.state = task.TaskStatus.SLEEPING;
		self.unlock();
	}

	fn get_queue_level(self: *Scheduler, queue: ?*?*task.Task) i64 {
		if(queue) |q| {
			for(0..self.queues.len) |i| {
				if(q == &self.queues[i]) return @intCast(i);
			} else return -1;
		} else return -1;
	}

	fn tasks_queued_for_execution(self: *Scheduler) bool {
		for(self.queues) |queue| {
			if(queue != null) return true;
		} else return false;
	}

	fn first_task_with_higher_priority(self: *Scheduler, priority: i8) ?*task.Task {
		if(priority < 0) return null;
		for(0..@intCast(priority)) |i| {
			if(self.queues[i] != null) return self.queues[i];
		} else return null;
	}

	fn move_task_down(self: *Scheduler, tsk: *task.Task) void {
		const tsk_level = self.get_queue_level(tsk.current_queue);
		if(tsk_level >= 0 and tsk_level < (queue_len - 1)) {
			self.remove_task_from_list(tsk, tsk.current_queue.?);
			self.add_task_to_list(tsk, &self.queues[@intCast(tsk_level + 1)]);
		}
	}

	// Assumes tasks is a circular linked list (which should be with how processes are allocated)
	fn search_available_task(self: *Scheduler, start_at: ?*task.Task) ?*task.Task {
		if(start_at) |t| {
			const queue_level: i64 = self.get_queue_level(t.current_queue.?);
			const level: usize = switch (queue_level == -1) {
				true => self.queues.len,
				false => @intCast(queue_level),
			};
			const task_with_higher_priority = self.first_task_with_higher_priority(@intCast(level));
			const next = t.next;
			if(task_with_higher_priority) |h| {return h;} else return next;
		} else {
			const task_with_higher_priority = self.first_task_with_higher_priority(@intCast(self.queues.len));
			if(task_with_higher_priority) |t| {return t;} else return null;
		}
		return null;
	}

	// Needs to have a idle task set up, or it will panic
	fn next_task(self: *Scheduler) *task.Task {
		// Cleanup task has the highest priority. Why? because memory is limited. We need to free the dead process as soon as possible.
		if(self.cleanup_task) |t| {
			if(t.state == task.TaskStatus.READY) return t;
		}
		// Basically:
		//    - If current process is a special task (either idle or cleanup) or finished search for another task. If there isn't one, just return the idle
		//    - If current process is a normal task, just check if hasn't finished. If it is, search for another task (if no tasks available return the idle)
		//    - If it hasn't finished, return the next task.
		if(self.current_process) |proc| {
			const task_param = switch(proc == self.idle_task.? or proc == self.cleanup_task.? or proc.state == task.TaskStatus.FINISHED) {
				true => null,
				false => proc
			};
			if(self.search_available_task(task_param)) |t| {
				return t;
			}
		}
		return self.idle_task.?;
	}

	pub fn clear_deleted_tasks(self: *Scheduler) void {
		self.lock();
		if(self.finished_tasks) |_| {
			var tptr = self.finished_tasks;
			while(self.finished_tasks != null) : (tptr = self.finished_tasks) {
				self.remove(tptr.?);
			}
		}
		self.unlock();
	}

	pub fn schedule(self: *Scheduler) void {
		self.lock();
		// If current process has been assigned (by setting up an idle task)
		// then we just search for the next one.
		if(self.current_process != null) {
			const t = self.next_task();
			self.move_task_down(self.current_process.?);
			self.copy_iobitmap(t);
			switch_task(
				&self.current_process.?,
				t,
				self.cpu_tss,
				@intFromBool(self.current_process.?.state == task.TaskStatus.FINISHED));
		}
		self.unlock();
	}

	pub fn block(self: *Scheduler, tsk: *task.Task, reason: task.TaskStatus) void {
		self.lock();
		// If it was unlocked, we need to move it to the correct list.
		tsk.state = reason;
		if(tsk != self.idle_task.? and tsk != self.cleanup_task.?) {
			self.remove_task_from_list(tsk, tsk.current_queue.?);
			const list = switch(reason) {
				task.TaskStatus.FINISHED => &self.finished_tasks,
				else => &self.blocked_tasks
			};
			self.add_task_to_list(tsk, list);
		}
		if(tsk == self.current_process.?) {
			self.schedule();
		}
		self.unlock();
	}
	pub fn unblock(self: *Scheduler, tsk: *task.Task) void {
		self.lock();
		// If it was locked, we need to move it to the correct list.
		// If it wasn't locked, this would be a no-op.
		const was_locked = tsk.state != task.TaskStatus.READY;
		tsk.state = task.TaskStatus.READY;
		if(tsk != self.idle_task.? and tsk != self.cleanup_task.? and was_locked) {
			// Move the task from
			self.remove_task_from_list(tsk, &self.blocked_tasks);
			self.add_task_to_list(tsk, &self.queues[0]);
		}
		if(self.current_process.? == self.idle_task.? or self.first_task_with_higher_priority(self.get_priority(self.current_process.?)) != null) {
			// Preempt the lower priority tasks if a higher priority task is running
			self.schedule();
		}
	
		self.unlock();
	}
	pub fn exit(self: *Scheduler, tsk: *task.Task) void {
		self.lock();
		// Unblock the cleanup task to free the deleted task's resources
		if(self.cleanup_task != null) {
			self.unblock(self.cleanup_task.?);
		}
		// Mark the task as finished
		self.block(tsk, task.TaskStatus.FINISHED);
		self.unlock();
	}

	fn remove(self: *Scheduler, tsk: *task.Task) void {
		// As when finalizing tasks we call block(), the task is in the
		// blocked_tasks list
		self.remove_task_from_list(tsk, tsk.current_queue.?);
		if(tsk.deinitfn) |_| {
			tsk.deinitfn.?(tsk, tsk.extra_arg);
		}
	}

	fn add_task_to_list(self: *Scheduler, tas: *task.Task, list: *?*task.Task) void {
		_ = self;
		// Two cases: there is a node and no node
		if(list.* != null) {
			// Add at the end the list
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
		tas.current_queue = list;
	}

	fn remove_task_from_list(self: *Scheduler, tsk: *task.Task, list: *?*task.Task) void {
		_ = self;
		if(list.*) |proc| {
			if(tsk.next.? == tsk and tsk.prev.? == tsk) { // Only one element in the list
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

	pub fn get_priority(self:* Scheduler, tsk: *task.Task) i8 {
		return for(0..self.queues.len) |i| {
			if(tsk.current_queue == &self.queues[i]) {break @truncate(@as(i64, @intCast(i)));}
		} else -1;
	}

	fn copy_iobitmap(self: *Scheduler, tsk: *task.Task) void {
		if(tsk.iopb_bitmap) |bitmap| {
			@memcpy(&self.cpu_tss.io_bitmap, bitmap);
			self.cpu_tss.iopb = @sizeOf(@TypeOf(self.cpu_tss.*)) - @sizeOf(@TypeOf(self.cpu_tss.io_bitmap));
		} else {
			self.cpu_tss.iopb = 65535;
		}
	}
};
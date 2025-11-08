const task = @import("task.zig");
const ts = @import("../memory/tss.zig");
const std = @import("std");
const idt = @import("../interrupts/idt.zig");
const spin = @import("../sync/spinlock.zig");
const tm = @import("timerman.zig");
const lb = @import("loadbalancer.zig");
const main = @import("../main.zig");
const fpu = @import("fpu_buffer_alloc.zig");

pub const queue_len = 4;
pub const LOAD_AVG_TICK_SIZE = 256.0;
const CONTEXT_SWITCH_TICKS = 20; // 20 ms between task switches

extern fn switch_task(
	current_task: **task.Task,
	next_task: *task.Task,
	tss: *ts.tss_t,
	current_task_deleted: u64,
	) callconv(.C) void;

pub const scheduler_queue = struct {
	head: ?*task.Task,
	count: std.atomic.Value(u32),
	fn init() @This() {
		return .{.head = null, .count = std.atomic.Value(u32).init(0)};
	}
};
// NOTE: ONE SCHEDULER PER CPU CORE
// Note2: To ask any other core for anything else, look at the IPI protocol.
pub const Scheduler = struct {
	queues: [queue_len]scheduler_queue,
	current_process: ?*task.Task,
	blocked_tasks: scheduler_queue,
	sleeping_tasks: scheduler_queue,
	finished_tasks: scheduler_queue,
	idle_task: ?*task.Task,
	cleanup_task: ?*task.Task,
	priority_boost_task: ?*task.Task,
	cpu_tss: *ts.tss_t,
	ntick: u32,
	timerman: tm.TimerManager,
	tick: u64,

	pub fn init(cpu_tss: *ts.tss_t) Scheduler {
		return .{
			.idle_task = null,
			.current_process = null,
			.blocked_tasks = scheduler_queue.init(),
			.sleeping_tasks = scheduler_queue.init(),
			.queues = [_]scheduler_queue{scheduler_queue.init()} ** queue_len,
			.finished_tasks = scheduler_queue.init(),
			.cleanup_task = null,
			.priority_boost_task = null,
			.cpu_tss = cpu_tss,
			.ntick = 0,
			.timerman = tm.TimerManager.init(),
			.tick = 0,
		};
	}


	pub fn get_load(self: *Scheduler) u32 {
		var ret: u32 = 0;
		for (self.queues) |q| {
			ret += q.count.load(.acquire);
		}
		return ret;
	}

	pub fn lock(_: *Scheduler) void {
		idt.disable_interrupts();
	}
	pub fn unlock(_: *Scheduler) void {
		idt.enable_interrupts();
	}
	// Adds a new task
	pub fn add_task(self: *Scheduler, tas: *task.Task) void {
		self.lock();
		self.add_task_to_list(tas, &self.queues[0]);
		self.unlock();
	}
	// Adds a priority boost task
	pub fn add_priority_boost(self: *Scheduler, tas: *task.Task) void {
		self.lock();
		self.priority_boost_task = tas;
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

	fn get_queue_level(self: *Scheduler, queue: ?*scheduler_queue) i64 {
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
		const prior = if(priority < 0) 4 else priority;
		for(0..@intCast(prior)) |i| {
			if(self.queues[i].head != null) return self.queues[i].head;
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
			const queue_level: i64 = if(t.current_queue != null) self.get_queue_level(t.current_queue.?) else -1;
			const level: usize = switch (queue_level == -1) {
				true => self.queues.len,
				false => @intCast(queue_level),
			};
			const task_with_higher_priority = self.first_task_with_higher_priority(@intCast(level));
			const next = t.next;
			if(task_with_higher_priority != null) {return task_with_higher_priority.?;} else return next;
		} else {
			const task_with_higher_priority = self.first_task_with_higher_priority(@intCast(self.queues.len));
			if(task_with_higher_priority != null) {return task_with_higher_priority.?;} else return null;
		}
		return null;
	}

	// Needs to have a idle task set up, or it will panic
	fn next_task(self: *Scheduler) *task.Task {
		// Cleanup task has the highest priority. Why? because memory is limited. We need to free the dead process as soon as possible.
		if(self.cleanup_task) |t| {
			if(t.state == task.TaskStatus.READY) return t;
		}
		// Second highest priority goes to the priority boost task
		if(self.priority_boost_task) |t| {
			if(t.state == task.TaskStatus.READY) return t;
		}
		// Basically:
		//    - If current process is a special task (either idle or cleanup) or finished search for another task. If there isn't one, just return the idle
		//    - If current process is a normal task, just check if hasn't finished. If it is, search for another task (if no tasks available return the idle)
		//    - If it hasn't finished, return the next task.
		if(self.current_process) |proc| {
			const task_param = switch(proc == self.idle_task.? or proc == self.cleanup_task.? or proc.state != task.TaskStatus.READY) {
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
		if(self.finished_tasks.head) |_| {
			var tptr = self.finished_tasks.head;
			while(self.finished_tasks.head != null) : (tptr = self.finished_tasks.head) {
				self.remove(tptr.?);
			}
		}
		self.unlock();
	}

	pub fn sleep(self: *Scheduler, ms: u64, tsk: *task.Task) void {
		self.lock();
		self.timerman.new_timer(ms, tsk);
		self.unlock();
		self.block(tsk, task.TaskStatus.SLEEPING);
	}

	pub fn on_irq_tick(self: *Scheduler) void {
		idt.disable_interrupts();
		self.timerman.on_tick(self) catch {};
		self.lock();
		self.ntick = (self.ntick + 1) % CONTEXT_SWITCH_TICKS;
		self.tick += 1;
		const should_schedule = self.ntick == 0;
		self.unlock();
		if(should_schedule) self.schedule(true);
	}

	pub fn schedule(self: *Scheduler, on_interrupt: bool) void {
		self.lock();
		// If current process has been assigned (by setting up an idle task)
		// then we just search for the next one.
		if(self.current_process != null) {
			const t = self.next_task();
			if(on_interrupt) self.move_task_down(self.current_process.?);
			if(self.current_process == t) {
				self.unlock();
				return;
			}
			self.copy_iobitmap(t);
			if(self.current_process.?.has_used_vector) fpu.save_fpu_buffer(self.current_process.?);
			switch_task(
				&self.current_process.?,
				t,
				self.cpu_tss,
				@intFromBool(self.current_process.?.state == task.TaskStatus.FINISHED));
		} else {
			// switch_task unlocks the scheduler at the end
			self.unlock();
		}
	}

	pub fn block(self: *Scheduler, tsk: *task.Task, reason: task.TaskStatus) void {
		self.lock();
		// If it was unlocked, we need to move it to the correct list.
		tsk.state = reason;
		if(tsk != self.idle_task.? and tsk != self.cleanup_task.? and tsk != self.priority_boost_task) {
			self.remove_task_from_list(tsk, tsk.current_queue.?);
			const list = switch(reason) {
				task.TaskStatus.FINISHED => &self.finished_tasks,
				task.TaskStatus.SLEEPING => &self.sleeping_tasks,
				else => &self.blocked_tasks
			};
			self.add_task_to_list(tsk, list);
		}
		const should_schedule = tsk == self.current_process.?;
		self.unlock();
		if(should_schedule) {
			self.schedule(false);
		}
	}

	pub fn unblock_without_scheduling(self: *Scheduler, tsk: *task.Task) void {
		self.lock();
		// If it was locked, we need to move it to the correct list.
		// If it wasn't locked, this would be a no-op.
		const was_locked = tsk.state != task.TaskStatus.READY;
		tsk.state = task.TaskStatus.READY;
		if(tsk != self.idle_task.? and tsk != self.cleanup_task.? and tsk != self.priority_boost_task and was_locked) {
			// Move the task from
			self.remove_task_from_list(tsk, tsk.current_queue.?);
			self.add_task_to_list(tsk, &self.queues[0]);
		}
		self.unlock();
	}
	pub fn unblock(self: *Scheduler, tsk: *task.Task) void {
		self.unblock_without_scheduling(tsk);
		self.lock();
		const schedule_next = self.current_process.? == self.idle_task.? or self.first_task_with_higher_priority(self.get_priority(self.current_process.?)) != null;
		self.unlock();
		if(schedule_next) {
			// Preempt the lower priority tasks if a higher priority task is running
			self.schedule(false);
		}
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
		// First we remove the task from its queue
		self.remove_task_from_list(tsk, tsk.current_queue.?);
		if(tsk.deinitfn) |_| {
			tsk.deinitfn.?(tsk, tsk.extra_arg);
		}
	}

	pub fn add_task_to_list(self: *Scheduler, tas: *task.Task, list: *scheduler_queue) void {
		_ = self;
		// Two cases: there is a node and no node
		if(list.head != null) {
			// Add at the end the list
			var head = list.head.?;
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
			list.head = tas;
			list.head.?.next = list.head;
			list.head.?.prev = list.head;
		}
		_ = list.count.fetchAdd(1, .acq_rel);
		tas.current_queue = list;
	}

	pub fn add_task_to_front(self: *Scheduler, tas: *task.Task, list: *scheduler_queue) void {
		_ = self;
		// Two cases: there is a node and no node
		if(list.head != null) {
			// Add at the front of the list
			var head = list.head.?;
			var last = head.prev.?;
			var data = tas;
			data.next = head;
			data.prev = last;
			head.prev = data;
			last.next = data;
			//head = data;
		} else {
			// Basically:
			// - add the node
			// - make the node point to itself
			list.head = tas;
			list.head.?.next = list.head;
			list.head.?.prev = list.head;
		}
		_ = list.count.fetchAdd(1, .acq_rel);
		tas.current_queue = list;
	}

	pub fn remove_task_from_list(self: *Scheduler, tsk: *task.Task, list: *scheduler_queue) void {
		_ = self;
		if(list.head) |proc| {
			if(tsk.next.? == tsk and tsk.prev.? == tsk) { // Only one element in the list
				list.head = null;
			} else { // More than one element in the list
				if(tsk == proc) {
					list.head = tsk.next;
				}
				if (tsk.prev) |prev| {
					prev.next = tsk.next;
				}
				if (tsk.next) |next| {
					next.prev = tsk.prev;
				}
			}
			tsk.next = null;
			tsk.prev = null;
			tsk.current_queue = null;
			_ = list.count.fetchSub(1, .acq_rel);
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
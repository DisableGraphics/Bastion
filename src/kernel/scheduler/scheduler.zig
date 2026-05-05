const task = @import("task.zig");
const ts = @import("../memory/tss.zig");
const std = @import("std");
const idt = @import("../interrupts/idt.zig");
const spin = @import("../sync/spinlock.zig");
const tm = @import("timerman.zig");
const lb = @import("loadbalancer.zig");
const main = @import("../main.zig");
const fpu = @import("fpu_buffer_alloc.zig");
const ppt = @import("../memory/per_process_table.zig");
const assm = @import("../arch/x86_64/asm.zig");
const log = @import("../log.zig");

pub const queue_len = 4;
pub const LOAD_AVG_TICK_SIZE = 256.0;
const CONTEXT_SWITCH_TICKS = 20; // 20 ms between task switches

// Switches the task currently running into next_task. Updates current_task pointer to point to next_task.
// If current task has been deleted (current_task_deleted != 0) it doesn't save registers.
// Also updates the per-process data with the correct info.
// Also platform-dependent and usually written in assembly.
// NOTE: this function unblocks the scheduler. 
extern fn switch_task(
	current_task: **task.Task,
	next_task: *task.Task,
	tss: *ts.tss_t,
	current_task_deleted: u64,
	ppt_t: *ppt.PerProcessData,
) callconv(.C) void;

pub const scheduler_queue = struct {
	head: ?*task.Task,
	count: std.atomic.Value(u32),
	fn init() @This() {
		return .{.head = null, .count = std.atomic.Value(u32).init(0)};
	}

	fn insert_at_end(self: *@This(), tsk: *task.Task) void {
		// Two cases: there is a node and no node
		if(self.head != null) {
			// Add at the end the list
			var head = self.head.?;
			var last = head.prev.?;
			var data = tsk;
			data.next = head;
			data.prev = last;
			head.prev = data;
			last.next = data;
		} else {
			// Basically:
			// - add the node
			// - make the node point to itself
			self.head = tsk;
			self.head.?.next = self.head;
			self.head.?.prev = self.head;
		}
		_ = self.count.fetchAdd(1, .acq_rel);
		tsk.current_queue = self;
	}

	fn insert_at_start(self: *@This(), tsk: *task.Task) void {
		// Two cases: there is a node and no node
		if(self.head != null) {
			// Add at the front of the list
			var head = self.head.?;
			var last = head.prev.?;
			var data = tsk;
			data.next = head;
			data.prev = last;
			head.prev = data;
			last.next = data;
			head = data;
		} else {
			// Basically:
			// - add the node
			// - make the node point to itself
			self.head = tsk;
			self.head.?.next = self.head;
			self.head.?.prev = self.head;
		}
		_ = self.count.fetchAdd(1, .acq_rel);
		tsk.current_queue = self;
	}

	fn remove(self: *@This(), tsk: *task.Task) void {
		if(self.head) |proc| {
			if(tsk.next.? == tsk and tsk.prev.? == tsk) { // Only one element in the list
				self.head = null;
			} else { // More than one element in the list
				if(tsk == proc) {
					self.head = tsk.next;
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
			_ = self.count.fetchSub(1, .acq_rel);
		}
	}
};
// NOTE: ONE SCHEDULER PER CPU CORE
// Note2: To ask any other core for anything else, look at the IPI protocol.
pub const Scheduler = struct {
	queues: [queue_len]scheduler_queue, // Queues to keep the pending tasks
	current_process: ?*task.Task, // Current task that's running right now
	blocked_tasks: scheduler_queue, // List of blocked tasks
	sleeping_tasks: scheduler_queue, // List of sleeping tasks
	finished_tasks: scheduler_queue, // List of finished tasks with cleanup pending
	idle_task: ?*task.Task, // Idle task (might or might not be executing)
	cleanup_task: ?*task.Task, // Cleanup task
	priority_boost_task: ?*task.Task, // Priority boost task (every n ticks, boost priority of all the tasks in the queues)
	cpu_tss: *ts.tss_t, // Pointer to the TSS in the current CPU
	ntick: u32, // Tick counter that wraps
	timerman: tm.TimerManager,
	tick: u64, // Actual tick counter
	has_transferred_ok: std.atomic.Value(bool), // If a requested task has transferred correctly
	flags: u64 = 0, // Flags to implement interrupt locking

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
			.has_transferred_ok = std.atomic.Value(bool).init(true)
		};
	}


	pub fn get_load(self: *Scheduler) u32 {
		var ret: u32 = 0;
		for (self.queues) |q| {
			ret += q.count.load(.acquire);
		}
		return ret;
	}

	pub fn lock(self: *Scheduler) void {
		if(self.flags != 0) {
			std.debug.panic("Tried to lock an already locked scheduler from (addr: {x}) (flags: {x}) (cpuid: {})", .{@returnAddress(), self.flags, main.mycpuid()});
		}
		//idt.disable_interrupts();
		self.flags = assm.irqdisable();
	}
	pub fn unlock_without_enabling_int(self: *Scheduler) void {
		self.flags = 0;
	}
	pub fn unlock(self: *Scheduler) void {
		//idt.enable_interrupts();
		const flags = self.flags;
		self.flags = 0;
		assm.irqrestore(flags);
	}
	// Adds a new task
	pub fn add_task(self: *Scheduler, tas: *task.Task) void {
		self.lock();
		self.add_task_to_list(tas, &self.queues[0]);
		self.unlock();
	}
	// Adds a new task without cheking for the lock
	pub fn add_task_with_lock_held(self: *Scheduler, tas: *task.Task) void {
		self.add_task_to_list(tas, &self.queues[0]);
	}
	// Adds a new blocked task
	pub fn add_blocked_task(self: *Scheduler, tas: *task.Task) void {
		self.lock();
		self.add_task_to_list(tas, &self.blocked_tasks);
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
			if(queue.head != null) return true;
		} else return false;
	}

	fn first_task_with_higher_priority(self: *Scheduler, priority: i8) ?*task.Task {
		const prior = if(priority < 0) queue_len else priority;
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
			if(task_with_higher_priority != null) {
				return task_with_higher_priority.?;
			} else {
				if(queue_level == -1) {
					return null;
				} else {
					return next;
				}
			}
		} else {
			const task_with_higher_priority = self.first_task_with_higher_priority(@intCast(self.queues.len));
			if(task_with_higher_priority != null) {return task_with_higher_priority.?;} else return null;
		}
		return null;
	}

	// Needs to have a idle task set up, or it will panic
	fn next_task(self: *Scheduler) *task.Task {
		// High priority goes to the priority boost task
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
		if(self.cleanup_task) |t| {
			if(t.state == task.TaskStatus.READY) return t;
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
		self.block_with_lock_held(tsk, task.TaskStatus.SLEEPING);
		// If the destination isn't the task currently running in this scheduler
		// then I don't need to schedule myself because there can only be 1 task
		// running.
		const should_schedule = tsk == self.current_process.?;
		if(should_schedule) self.schedule_with_lock_held(false) else self.unlock();
	}

	pub fn on_irq_tick(self: *Scheduler) void {
		self.lock();
		self.timerman.on_tick(self) catch {};
		self.ntick = (self.ntick + 1) % CONTEXT_SWITCH_TICKS;
		self.tick += 1;
		const should_schedule = self.ntick == 0;
		if(should_schedule) self.schedule_with_lock_held(true) else self.unlock();
	}

	pub fn schedule_with_lock_held(self: *Scheduler, on_interrupt: bool) void {
		// If current process has been assigned (by setting up an idle task)
		// then we just search for the next one.
		if(self.current_process != null) {
			const t = self.next_task();
			if(on_interrupt) self.move_task_down(self.current_process.?);
			self.schedule_to(t);
		} else {
			// switch_task unlocks the scheduler at the end
			self.unlock();
		}
	}

	pub fn schedule_to(self: *Scheduler, t: *task.Task) void {
		if(self.current_process == t) {
			self.unlock();
			return;
		}
		self.copy_iobitmap(t);
		if(self.current_process.?.has_used_vector) fpu.save_fpu_buffer(self.current_process.?);
		self.current_process.?.save_tls();
		const mytable = ppt.PerProcessTable.get_my_table();
		switch_task(
			&self.current_process.?,
			t,
			self.cpu_tss,
			@intFromBool(self.current_process.?.state == task.TaskStatus.FINISHED),
			mytable);
	}

	pub fn schedule(self: *Scheduler, on_interrupt: bool) void {
		self.lock();
		self.schedule_with_lock_held(on_interrupt);
	}

	fn block_with_lock_held(self: *Scheduler, tsk: *task.Task, reason: task.TaskStatus) void {
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
	}

	pub fn block(self: *Scheduler, tsk: *task.Task, reason: task.TaskStatus) void {
		self.lock();
		self.block_with_lock_held(tsk, reason);
		// If the destination isn't the task currently running in this scheduler
		// then I don't need to schedule myself because there can only be 1 task
		// running.
		const should_schedule = tsk == self.current_process.?;
		if(should_schedule) self.schedule_with_lock_held(false) else self.unlock();
	}

	pub fn unblock_without_scheduling(self: *Scheduler, tsk: *task.Task) void {
		std.log.debug("unblock_without_scheduling: {x}", .{@returnAddress()});
		// If it was locked, we need to move it to the correct list.
		// If it wasn't locked, this would be a no-op.
		const was_locked = tsk.state != task.TaskStatus.READY;
		tsk.state = task.TaskStatus.READY;
		if(tsk != self.idle_task.? and tsk != self.cleanup_task.? and tsk != self.priority_boost_task and was_locked) {
			// Move the task from
			if (tsk.current_queue) |q| {
				self.remove_task_from_list(tsk, q);
			}
			self.add_task_to_list(tsk, &self.queues[0]);
		}
	}
	
	pub fn unblock(self: *Scheduler, tsk: *task.Task) void {
		std.log.debug("unblock: {x}", .{@returnAddress()});
		self.lock();
		self.unblock_without_scheduling(tsk);
		const schedule_next = self.current_process.? == self.idle_task.? or self.first_task_with_higher_priority(self.get_priority(self.current_process.?)) != null;
		if(schedule_next) self.schedule_with_lock_held(false) else self.unlock();
	}

	pub fn exit(self: *Scheduler, tsk: *task.Task) void {
		self.lock();
		// Unblock the cleanup task to free the deleted task's resources
		if(self.cleanup_task != null) {
			self.unblock_without_scheduling(self.cleanup_task.?);
		}
		// Mark the task as finished
		self.block_with_lock_held(tsk, task.TaskStatus.FINISHED);
		const should_schedule = tsk == self.current_process.?;
		if(should_schedule) self.schedule_with_lock_held(false) else self.unlock();
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
		list.insert_at_end(tas);
	}

	pub fn add_task_to_front(self: *Scheduler, tas: *task.Task, list: *scheduler_queue) void {
		_ = self;
		list.insert_at_start(tas);
	}

	pub fn remove_task_from_list(self: *Scheduler, tsk: *task.Task, list: *scheduler_queue) void {
		_ = self;
		list.remove(tsk);
	}

	pub fn get_priority(self:* Scheduler, tsk: *task.Task) i8 {
		return for(0..self.queues.len) |i| {
			if(tsk.current_queue == &self.queues[i]) {break @truncate(@as(i64, @intCast(i)));}
		} else -1;
	}

	pub fn copy_iobitmap(self: *Scheduler, tsk: *task.Task) void {
		ts.io_bitmap_on_task_switch(tsk.iopb_bitmap, self.cpu_tss);
	}
};
const tsk = @import("task.zig");
const all = @import("timeralloc.zig");
const sch = @import("scheduler.zig");
const spin = @import("../sync/spinlock.zig");
const std = @import("std");

pub const Timer = struct {
	expires: u64,
	next: ?*Timer,
	proc: *tsk.Task,
};

pub const TimerWheel = struct {
	pub const WHEEL_SIZE = 256;
	slots: [TimerWheel.WHEEL_SIZE]?*Timer,
	pub fn init() TimerWheel {
		return .{
			.slots = [_]?*Timer{null} ** TimerWheel.WHEEL_SIZE
		};
	}
};

const WHEEL_LEVELS = 4;

pub const TimerManager = struct {
	wheels: [WHEEL_LEVELS]TimerWheel,
	tick: u64,
	pub fn init() TimerManager {
		return .{
			.wheels = [_]TimerWheel{TimerWheel.init()} ** WHEEL_LEVELS,
			.tick = 0
		};
	}

	pub fn new_timer(self: *TimerManager, diff_ticks: u64, task: *tsk.Task) void {
		const timer = all.TimerAllocator.alloc();
		if(timer) |t| {
			t.expires = self.tick + diff_ticks;
			t.proc = task;
			t.next = null;
			self.add(t);
		}
	}

	fn add(self: *TimerManager, t: *Timer) void {
		const mask = TimerWheel.WHEEL_SIZE - 1;
		var lvl: u6 = 0;
		var diff: u64 = if (t.expires > self.tick)
			t.expires - self.tick
		else
			0;

		var max_for_level: usize = TimerWheel.WHEEL_SIZE;
		while (lvl < WHEEL_LEVELS - 1 and diff >= max_for_level) {
			lvl+=1;
			diff >>= 8;
			max_for_level <<= 8;
		}
		const slot = (t.expires >> (lvl << 3)) & mask;
		t.next = self.wheels[lvl].slots[slot];
		self.wheels[lvl].slots[slot] = t;
	}

	fn cascade(self: *TimerManager, lvl: u6) void {
		const slot = ((self.tick - 1) >> (@as(u6, @intCast(lvl)) << 3)) & (TimerWheel.WHEEL_SIZE - 1);
    	var t = self.wheels[lvl].slots[slot];
   		self.wheels[lvl].slots[slot] = null;
		
		while (t != null) {
			const next = t.?.next;
			self.add(t.?);
			t = next;
		}
	}

	pub fn on_tick(self: *TimerManager, sched: *sch.Scheduler) !void {
		const slot = self.tick & (TimerWheel.WHEEL_SIZE - 1);
		var timer = self.wheels[0].slots[slot];
		self.wheels[0].slots[slot] = null;
		while(timer != null) {
			const next = timer.?.next;
			sched.unblock_without_scheduling(timer.?.proc);
			try all.TimerAllocator.free(timer.?);
			timer = next;
		}
		self.tick += 1;
		if(slot == 0) {
			for(1..WHEEL_LEVELS) |lvl| {
				const l: u6 = @intCast(lvl);
				const mask = TimerWheel.WHEEL_SIZE - 1;
				if ((((self.tick - 1) >> ((l-1) << 3)) & mask) != 0) break;
				self.cascade(l);
			}
		}
	}
};
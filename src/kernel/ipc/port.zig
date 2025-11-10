const tsk = @import("../scheduler/task.zig");
const std = @import("std");
const spin = @import("../sync/spinlock.zig");

pub const Rights = struct {
	pub const SEND = 1;
	pub const RECV = (1 << 1);
	pub const TRANSF = (1 << 2);
	pub const KILL = (1 << 3);
	pub const MODIF_RIGHTS = (1 << 4);
};

pub const Port = struct {
	owner: std.atomic.Value(?*tsk.Task) = std.atomic.Value(?*tsk.Task).init(null),
	rights_mask: std.atomic.Value(u8) = std.atomic.Value(u8).init(1),
	count: std.atomic.Value(u32) = std.atomic.Value(u32).init(0),
	cpu_owner: std.atomic.Value(u32) = std.atomic.Value(u32).init(0),
	lock: spin.SpinLock = spin.SpinLock.init(),

	send_head: ?*tsk.Task = null,
	send_tail: ?*tsk.Task = null,
	recv_head: ?*tsk.Task = null,
	recv_tail: ?*tsk.Task = null,

	pub fn enqueueSender(self: *Port, task: *tsk.Task) void {
		task.next = null;
		task.prev = self.send_tail;
		if (self.send_tail) |tail| { tail.next = task; } else self.send_head = task;
		self.send_tail = task;
	}

	pub fn dequeueSender(self: *Port) ?*tsk.Task {
		const head = self.send_head orelse return null;
		self.send_head = head.next;
		if (self.send_head) |h| h.prev = null else self.send_tail = null;
		head.next = null;
		head.prev = null;
		return head;
	}

	pub fn enqueueReceiver(self: *Port, task: *tsk.Task) void {
		task.next = null;
		task.prev = self.recv_tail;
		if (self.recv_tail) |tail| { tail.next = task; } else self.recv_head = task;
		self.recv_tail = task;
	}

	pub fn dequeueReceiver(self: *Port) ?*tsk.Task {
		const head = self.recv_head orelse return null;
		self.recv_head = head.next;
		if (self.recv_head) |h| h.prev = null else self.recv_tail = null;
		head.next = null;
		head.prev = null;
		return head;
	}
};
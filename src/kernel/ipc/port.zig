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
	rights_mask: std.atomic.Value(u64) = std.atomic.Value(u64).init(Rights.SEND),
	count: std.atomic.Value(u32) = std.atomic.Value(u32).init(0),
	cpu_owner: std.atomic.Value(u32) = std.atomic.Value(u32).init(0),
	lock: spin.SpinLock = spin.SpinLock.init(),
	target: ?*anyopaque = null,

	send_head: ?*tsk.Task = null,
	send_tail: ?*tsk.Task = null,
	recv_head: ?*tsk.Task = null,
	recv_tail: ?*tsk.Task = null,

	wait_head: ?*tsk.Task = null,
	wait_tail: ?*tsk.Task = null,

	pub fn init() Port {
		return .{
			.owner = std.atomic.Value(?*tsk.Task).init(null),
			.rights_mask = std.atomic.Value(u64).init(Rights.SEND),
			.count = std.atomic.Value(u32).init(0),
			.cpu_owner = std.atomic.Value(u32).init(0),
			.lock = spin.SpinLock.init(),
			.target = null,
			.send_head = null,
			.send_tail = null,
			.recv_head = null,
			.recv_tail = null,

			.wait_head = null,
			.wait_tail = null,
		};
	}

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

	pub fn enqueueWaiter(self: *Port, task: *tsk.Task) void {
		task.next = null;
		task.prev = self.wait_tail;
		if (self.wait_tail) |tail| { tail.next = task; } else self.wait_head = task;
		self.wait_tail = task;
	}

	pub fn dequeueWaiter(self: *Port) ?*tsk.Task {
		const head = self.wait_head orelse return null;
		self.wait_head = head.next;
		if (self.wait_head) |h| h.prev = null else self.wait_tail = null;
		head.next = null;
		head.prev = null;
		return head;
	}
};
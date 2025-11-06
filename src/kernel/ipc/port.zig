const tsk = @import("../scheduler/task.zig");
const std = @import("std");

pub const Rights = struct {
	pub const SEND = 1;
	pub const RECV = (1 << 1);
	pub const TRANSF = (1 << 2);
	pub const KILL = (1 << 3);
	pub const MODIF_RIGHTS = (1 << 4);
};

pub const Port = struct {
	owner: std.atomic.Value(?*tsk.Task),
	rights_mask: std.atomic.Value(u8),
};
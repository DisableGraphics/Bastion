const tsk = @import("../scheduler/task.zig");

const Rights = struct {
	const SEND = 1;
	const RECV = (1 << 1);
	const TRANSF = (1 << 2);
	const KILL = (1 << 3);
	const MODIF_RIGHTS = (1 << 4);
};

const Port = struct {
	owner: *tsk.Task,
	rights_mask: u5,
};
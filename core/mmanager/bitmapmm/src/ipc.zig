pub const ipc = @cImport(@cInclude("ipc.h"));
const mman = @cImport(@cInclude("mmanager.h"));

const commandType = enum(u16) {
	ALLOC_REQUEST = 1,
	FREE_REQUEST = 2,
	ALLOC_RANGE_REQUEST = 3,
	FREE_RANGE_REQUEST = 4,
	FB_REQUEST = 1024,
	DEV_REQUEST = 1025
};

const Command = struct {
	source: ipc.port_t,
	cmd: commandType,
	arg0: usize,
	arg1: usize
};

const CommandResult = union(enum){ cmd: Command, ecode: []const u8};

pub fn receive_command(port: ipc.port_t) CommandResult {
	var msg = ipc.ipc_message_t {
		.dest = port,
		.flags = ipc.IPC_FLAG_TRANSFER_PORT
	};
	const ecode = ipc.sys_ipc_recv(&msg);
	return switch (ecode)  {
		ipc.EOK => label: {
			if(msg.value0 == 0 or (msg.value0 > 4 and msg.value0 < 1024) or (msg.value0 > 1025)) {
				return CommandResult { .ecode = "Incorrect command" };
			}
			const cmd = CommandResult { 
				.cmd = Command {.source = msg.source, .cmd = @enumFromInt(msg.value0), .arg0 = msg.value1, .arg1 = msg.value1} 
			};
			break :label cmd;
		},
		ipc.ENOPERM => CommandResult { .ecode = "No permissions" },
		ipc.EINVALMSG => CommandResult { .ecode = "Invalid message"},
		ipc.ENOOWN => CommandResult { .ecode = "No owner"},
		ipc.EINVALOP => CommandResult { .ecode = "Invalid operation"},
		ipc.ENODEST => CommandResult { .ecode = "No destination"},
		else => CommandResult { .ecode = "Unknown error"}
	};
}

pub fn respond_with_allocation(dest: ipc.port_t, addr: usize, npages: usize) !void {
	const msg = ipc.ipc_message_t {
		.dest = dest,
		.flags = 0,
		.value0 = mman.RES_OK,
		.page = addr,
		.npages = npages
	};
	const ecode = ipc.sys_ipc_send(&msg);
	return switch (ecode)  {
		ipc.EOK => {},
		ipc.ENOPERM => error.ENOPERM,
		ipc.EINVALMSG => error.EINVALMSG,
		ipc.ENOOWN => error.ENOOWN,
		ipc.EINVALOP => error.EINVALOP,
		ipc.ENODEST => error.ENODEST,
		else => error.UNKNOWN,
	};
}

pub fn respond_with_error(dest: ipc.port_t) !void {
	const msg = ipc.ipc_message_t {
		.dest = dest,
		.flags = 0,
		.value0 = mman.RES_ERR,
	};
	const ecode = ipc.sys_ipc_send(&msg);
	return switch (ecode)  {
		ipc.EOK => {},
		ipc.ENOPERM => error.ENOPERM,
		ipc.EINVALMSG => error.EINVALMSG,
		ipc.ENOOWN => error.ENOOWN,
		ipc.EINVALOP => error.EINVALOP,
		ipc.ENODEST => error.ENODEST,
		else => error.UNKNOWN,
	};
}

pub fn respond_ok(dest: ipc.port_t) !void {
	const msg = ipc.ipc_message_t {
		.dest = dest,
		.flags = 0,
		.value0 = mman.RES_OK,
	};
	const ecode = ipc.sys_ipc_send(&msg);
	return switch (ecode)  {
		ipc.EOK => {},
		ipc.ENOPERM => error.ENOPERM,
		ipc.EINVALMSG => error.EINVALMSG,
		ipc.ENOOWN => error.ENOOWN,
		ipc.EINVALOP => error.EINVALOP,
		ipc.ENODEST => error.ENODEST,
		else => error.UNKNOWN,
	};
}
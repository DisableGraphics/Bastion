const page = @import("../memory/pagemanager.zig");
pub const TaskStatus = enum(u64) {
	READY,
	RUNNING,
	SLEEPING,
	FINISHED
};

pub const Task = extern struct {
	stack: *anyopaque,
	root_page_table: *page.page_table_type,
	kernel_stack: *anyopaque,
	next: ?*Task,
	state: TaskStatus,

	pub fn init_kernel_task(
		func: *const fn() void,
		stack: *anyopaque, 
		kernel_stack: *anyopaque,
		root_page_table: *page.page_table_type,
		) Task {

		var stack_p: [*]usize = @ptrCast(@alignCast(stack));
		stack_p = stack_p - 8;
		stack_p[7] = @intFromPtr(func);
		stack_p[0] = 0x202;
		_ = kernel_stack;

		return .{
			.stack = @ptrCast(stack_p),
			.kernel_stack = @ptrCast(stack_p),
			.root_page_table = root_page_table,
			.next = null,
			.state = TaskStatus.READY
		};
	}

	pub fn init_idle_task(
		rsp: usize, cr3: usize
	) Task {
		return .{
			.stack = @ptrFromInt(rsp),
			.kernel_stack = @ptrFromInt(rsp),
			.root_page_table = @ptrFromInt(cr3),
			.next = null,
			.state = TaskStatus.READY
		};
	}
};
const page = @import("../memory/pagemanager.zig");
const kmm = @import("../memory/kmm.zig");
const std = @import("std");
pub const TaskStatus = enum(u64) {
	READY,
	SLEEPING,
	FINISHED
};

pub const KERNEL_STACK_SIZE = 16*1024;

pub const Task = extern struct {
	stack: *anyopaque,
	root_page_table: *page.page_table_type,
	kernel_stack: *anyopaque,
	next: ?*Task,
	prev: ?*Task,
	state: TaskStatus,
	kernel_stack_top: *anyopaque,
	deinitfn: ?*const fn(*Task, ?*anyopaque) void,
	extra_arg: ?*anyopaque,
	current_queue: ?*?*Task,

	pub fn format(
            self: @This(),
            comptime fmt: []const u8,
            options: std.fmt.FormatOptions,
            writer: anytype,
        ) !void {
            _ = fmt;
            _ = options;
            try writer.print("{}{{" ++ 
				".stack = {x}, " ++
				".root_page_table = {x}, " ++
				".kernel_stack = {x}, " ++ 
				".kernel_stack_top = {x}, " ++
				".next = {x}, " ++ 
				".prev = {x}, " ++ 
				".state = {s} }}", .{
				@This(),
               	@intFromPtr(self.stack),
               	@intFromPtr(self.root_page_table),
				@intFromPtr(self.kernel_stack),
				@intFromPtr(self.kernel_stack_top),
				@intFromPtr(self.next),
				@intFromPtr(self.prev),
				@tagName(self.state)
            });
        }

	pub fn init_kernel_task(
		func: *const fn() void,
		stack: *anyopaque, 
		kernel_stack: *anyopaque,
		root_page_table: *page.page_table_type,
		allocator: *kmm.KernelMemoryManager
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
			.prev = null,
			.state = TaskStatus.READY,
			.kernel_stack_top = @ptrFromInt(@intFromPtr(stack) - KERNEL_STACK_SIZE),
			.deinitfn = deinit_kernel_task,
			.extra_arg = @ptrCast(allocator),
			.current_queue = null
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
			.prev = null,
			.state = TaskStatus.READY,
			.kernel_stack_top = @ptrFromInt(16),
			.deinitfn = null,
			.extra_arg = null,
			.current_queue = null,
		};
	}

	fn deinit_kernel_task(self: *Task, arg: ?*anyopaque) void {
		std.log.info("Destroying kernel task {}", .{self});
		const kmmalloc: *kmm.KernelMemoryManager = @ptrCast(@alignCast(arg.?));
		kmmalloc.dealloc_virt(@intFromPtr(self.kernel_stack_top), KERNEL_STACK_SIZE/page.PAGE_SIZE) catch |err| {
			std.log.err("Could not free kernel stack for kernel task: {}", .{self});
			std.log.err("Error: {}", .{err});
		};
	}
};
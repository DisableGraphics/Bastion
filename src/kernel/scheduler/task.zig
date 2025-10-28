const page = @import("../memory/pagemanager.zig");
const kmm = @import("../memory/kmm.zig");
const std = @import("std");
const tss = @import("../memory/tss.zig");
const sa = @import("../memory/stackalloc.zig");
const main = @import("../main.zig");
const ipi = @import("../interrupts/ipi_protocol.zig");
const sch = @import("scheduler.zig");
const buffer = @import("fpu_buffer_alloc.zig");
const ta = @import("taskalloc.zig");

pub const TaskStatus = enum(u64) {
	READY,
	BLOCKED,
	SLEEPING,
	FINISHED
};

pub const KERNEL_STACK_SIZE = 16*1024;

pub const Task = extern struct {
	stack: *anyopaque,
	root_page_table: *page.page_table_type,
	kernel_stack: *sa.KernelStack,
	next: ?*Task = null,
	prev: ?*Task = null,
	state: TaskStatus,
	is_pinned: bool,
	kernel_stack_top: *anyopaque,
	deinitfn: ?*const fn(*Task, ?*anyopaque) void = null,
	extra_arg: ?*anyopaque = null,
	current_queue: ?*sch.scheduler_queue = null,
	iopb_bitmap: ?*tss.io_bitmap_t = null,
	cpu_created_on: u64,
	fpu_buffer: ?*buffer.fpu_buffer = null,
	cpu_fpu_buffer_created_on: u64 = 0,
	has_used_vector: bool = false,

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
		kernel_stack: *sa.KernelStack,
		root_page_table: *page.page_table_type,
		allocator: *kmm.KernelMemoryManager
		) Task {

		var stack_p: [*]usize = @ptrFromInt(@intFromPtr(kernel_stack) + @sizeOf(sa.KernelStack));
		stack_p = stack_p - 9;
		stack_p[8] = @intFromPtr(func);
		stack_p[0] = 0x202;

		return .{
			.stack = @ptrCast(stack_p),
			.kernel_stack = @ptrCast(stack_p),
			.root_page_table = root_page_table,
			.next = null,
			.prev = null,
			.state = TaskStatus.READY,
			.kernel_stack_top = @ptrCast(kernel_stack),
			.deinitfn = deinit_kernel_task,
			.extra_arg = @ptrCast(allocator),
			.current_queue = null,
			.iopb_bitmap = null,
			.is_pinned = true,
			.cpu_created_on = main.mycpuid(),
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
			.iopb_bitmap = null,
			.is_pinned = true,
			.cpu_created_on = main.mycpuid(),
		};
	}

	pub fn init_ipc_interrupt_task() Task {
		return .{
			.stack = @ptrFromInt(16),
			.kernel_stack = @ptrFromInt(16),
			.root_page_table = @ptrFromInt(16),
			.next = null,
			.prev = null,
			.state = TaskStatus.READY,
			.kernel_stack_top = @ptrFromInt(16),
			.deinitfn = null,
			.extra_arg = null,
			.current_queue = null,
			.iopb_bitmap = null,
			.is_pinned = true,
			.cpu_created_on = main.mycpuid(),
		};
	}

	fn deinit_kernel_task(self: *Task, _: ?*anyopaque) void {
		const mycpu = main.mycpuid();
		if(self.cpu_created_on == mycpu) {
			std.log.info("Destroying kernel task {}", .{self});
			sa.KernelStackAllocator.free(self.kernel_stack) catch |err| {
				std.log.err("Could not free kernel stack for kernel task: {}", .{self});
				std.log.err("Error: {}", .{err});
			};
		} else {
			ipi.IPIProtocolHandler.send_ipi(@truncate(self.cpu_created_on), ipi.IPIProtocolPayload.init_with_data(
				ipi.IPIProtocolMessageType.FREE_TASK,
				@intFromPtr(self),
				0,
				0
			));
		}

		if(self.fpu_buffer != null and self.cpu_fpu_buffer_created_on == mycpu) {
			std.log.info("Destroying FPU buffer for task task {}", .{self});
			buffer.FPUBufferAllocator.free(self.fpu_buffer.?) catch |err| {
				std.log.err("Could not free kernel stack for kernel task: {}", .{self});
				std.log.err("Error: {}", .{err});
			};
		} else if(self.fpu_buffer != null) {
			ipi.IPIProtocolHandler.send_ipi(@truncate(self.cpu_fpu_buffer_created_on), ipi.IPIProtocolPayload.init_with_data(
				ipi.IPIProtocolMessageType.FREE_FPU_BUFFER,
				@intFromPtr(self),
				0,
				0
			));
		}

		if(self.cpu_created_on == mycpu) {
			ta.TaskAllocator.free(self) catch |err| std.log.err("Error while freeing task: {}", .{err});
		}
	}
};
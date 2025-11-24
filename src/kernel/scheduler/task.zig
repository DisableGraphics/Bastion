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
const ioa = @import("../memory/io_bufferalloc.zig");
const port = @import("../ipc/port.zig");
const portchunk = @import("../ipc/portchunkalloc.zig");
const ips = @import("../ipc/ipcfn.zig");
const iport = @import("../interrupts/iporttable.zig");


pub const TaskStatus = enum(u64) {
	READY,
	IPC,
	BLOCKED,
	SLEEPING,
	FINISHED
};

pub const KERNEL_STACK_SIZE = 16*1024;
const N_PORTS = 4;
const N_PORT_CHUNKS = 8;

pub const Task = extern struct {
	stack: *anyopaque,
	root_page_table: *page.page_table_type,
	kernel_stack: *sa.KernelStack,
	next: ?*Task = null,
	prev: ?*Task = null,
	transfer_ok: ?*std.atomic.Value(bool) = null,
	kernel_stack_top: *anyopaque,
	deinitfn: ?*const fn(*Task, ?*anyopaque) void = null,
	extra_arg: ?*anyopaque = null,
	current_queue: ?*sch.scheduler_queue = null,
	iopb_bitmap: ?*tss.io_bitmap_t = null,
	fpu_buffer: ?*buffer.fpu_buffer = null,
	state: TaskStatus,
	iopb_bitmap_created_on: u32 = 0,
	cpu_created_on: u32 = 0,
	cpu_owner: u32 = 0,
	cpu_fpu_buffer_created_on: u32 = 0,
	has_used_vector: bool = false,
	is_pinned: bool,
	receive_msg: ?*ips.ipc_msg.ipc_message_t = null,
	send_msg: ?*const ips.ipc_msg.ipc_message_t = null,
	ports: [N_PORTS]?*port.Port = [_]?*port.Port{null} ** N_PORTS,
	port_chunks: [N_PORT_CHUNKS]?*portchunk.port_chunk = [_]?*portchunk.port_chunk{null} ** N_PORT_CHUNKS,
	irq_registered: u8 = std.math.maxInt(u8),

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
			.cpu_created_on = @truncate(main.mycpuid()),
			.cpu_owner = @truncate(main.mycpuid())
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
			.cpu_created_on = @truncate(main.mycpuid()),
			.cpu_owner = @truncate(main.mycpuid())
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
			.cpu_created_on = std.math.maxInt(u32),
			.cpu_owner = std.math.maxInt(u32)
		};
	}

	pub fn add_io_buffer(self: *@This()) !void {
		self.iopb_bitmap = ioa.IOBufferAllocator.alloc() orelse return error.OUT_OF_IO_BUFFER_SPACE;
		self.iopb_bitmap_created_on = @truncate(main.mycpuid());
	}

	pub fn register_for_irq(self: *@This(), pn: i16, irqn: u8) !void {
		self.irq_registered = irqn;
		const c = self.get_port(pn);
		if(c == null) return error.NO_PORT;
		try iport.InterruptPortTable.register_irq(c.?, irqn);
	}

	fn deinit_kernel_task(self: *Task, _: ?*anyopaque) void {
		const mycpu: u32 = @truncate(main.mycpuid());
		// First free FPU buffer
		if(self.fpu_buffer != null and self.cpu_fpu_buffer_created_on == mycpu) {
			std.log.info("Destroying FPU buffer for task task {}", .{self});
			buffer.FPUBufferAllocator.free(self.fpu_buffer.?) catch |err| {
				std.log.err("Could not free FPU buffer for kernel task: {}", .{self});
				std.log.err("Error: {}", .{err});
			};
		} else if(self.fpu_buffer != null) {
			ipi.IPIProtocolHandler.send_ipi(@truncate(self.cpu_fpu_buffer_created_on), ipi.IPIProtocolPayload.init_with_data(
				ipi.IPIProtocolMessageType.FREE_FPU_BUFFER,
				@intFromPtr(self.fpu_buffer.?),
				0,
				0
			));
		}
		// Free IO Bitmap
		if(self.iopb_bitmap != null and self.iopb_bitmap_created_on == mycpu) {
			std.log.info("Destroying IO buffer for task task {}", .{self});
			ioa.IOBufferAllocator.free(self.iopb_bitmap.?) catch |err| {
				std.log.err("Could not free IO bitmap for kernel task: {}", .{self});
				std.log.err("Error: {}", .{err});
			};
		} else if(self.iopb_bitmap != null) {
			ipi.IPIProtocolHandler.send_ipi(@truncate(self.iopb_bitmap_created_on), ipi.IPIProtocolPayload.init_with_data(
				ipi.IPIProtocolMessageType.FREE_IO_BITMAP,
				@intFromPtr(self.iopb_bitmap.?),
				0,
				0
			));
		}
		// Free all ports
		self.clear_all_ports() catch |err| {
			std.log.err("Error while freeing ports: {}", .{err});
		};

		// Free task
		if(self.cpu_created_on == mycpu) {
			std.log.info("Destroying kernel task {}", .{self});
			sa.KernelStackAllocator.free(self.kernel_stack) catch |err| {
				std.log.err("Could not free kernel stack for kernel task: {}", .{self});
				std.log.err("Error: {}", .{err});
			};
			ta.TaskAllocator.free(self) catch |err| std.log.err("Error while freeing task: {}", .{err});
		} else {
			ipi.IPIProtocolHandler.send_ipi(@truncate(self.cpu_created_on), ipi.IPIProtocolPayload.init_with_data(
				ipi.IPIProtocolMessageType.FREE_TASK,
				@intFromPtr(self),
				0,
				0
			));
		}
	}

	pub fn close_port(self: *Task, pn: i16) ?*port.Port {
		const len = N_PORT_CHUNKS*@typeInfo(portchunk.port_chunk).array.len;
		if(pn < 0 or pn > (N_PORTS + len)) return null;
		if(pn < N_PORTS) {
			const pn2: usize = @intCast(pn);
			var prt = self.ports[pn2];
			if(prt == null) return null;
			self.ports[pn2] = null;
			_ = prt.?.count.fetchSub(1, .seq_cst);
			return prt.?;
		}

		const pnr = pn - N_PORTS;
		const port_zone: usize = @intCast(@divTrunc(pnr, len));
		const port_no: usize = @intCast(@rem(pnr, len));
		if(self.port_chunks[port_zone] == null) return null;
		
		var ptr = self.port_chunks[port_zone].?[port_no];
		if(ptr == null) return null;
		self.port_chunks[port_zone].?[port_no] = null;
		_ = ptr.?.count.fetchSub(1, .seq_cst);
		return ptr.?;
	}

	pub fn add_port(self: *Task, prt: *port.Port) !i16 {
		_ = prt.count.fetchAdd(1, .acq_rel);
		for(0..self.ports.len) |i| {
			if(self.ports[i] == null) {
				self.ports[i] = prt;
				return @intCast(@as(u16, @truncate(i)));
			}
		}
		for(0..self.port_chunks.len) |n| {
			if(self.port_chunks[n] == null) {
				self.port_chunks[n] = portchunk.PortChunkAllocator.alloc();
				if(self.port_chunks[n] == null) return error.OUT_OF_PORT_CHUNKS;
			}
			for(0..self.port_chunks[n].?.len) |i| {
				if(self.port_chunks[n].?[i] == null) {
					self.port_chunks[n].?[i] = prt;
					const res = self.ports.len + (n - 1) * self.port_chunks.len + i;
					return @intCast(@as(u16, @truncate(res)));
				}
			}
		}
		return error.OUT_OF_PORT_SPACE;
	}

	pub fn get_port(self: *Task, pn: i16) ?*port.Port {
		const len = N_PORT_CHUNKS*@typeInfo(portchunk.port_chunk).array.len;
		if(pn < 0 or pn > (N_PORTS + len)) return null;
		if(pn < N_PORTS) {
			const pn2: usize = @intCast(pn);
			return self.ports[pn2];
		}
		const pnr = pn - N_PORTS;
		const port_zone: usize = @intCast(@divTrunc(pnr, len));
		const port_no: usize = @intCast(@rem(pnr, len));
		if(self.port_chunks[port_zone] == null) return null;
		return self.port_chunks[port_zone].?[port_no];
	}

	fn clear_all_ports(self: *Task) !void {
		const len = N_PORT_CHUNKS*@typeInfo(portchunk.port_chunk).array.len;
		for(0..(self.ports.len + len)) |i| {
			const casted: i16 = @intCast(i);
			if(self.get_port(casted) != null) {
				try ips.port_close(self, casted);
			}
		}
	}
};
const serial = @import("../serial/serial.zig");
const main = @import("../main.zig");
const std = @import("std");
const spin = @import("../sync/spinlock.zig");
const schman = @import("../scheduler/manager.zig");
const ipcfn = @import("../ipc/ipcfn.zig");

pub const IdtFrame = extern struct {
	ip: u64,
	cs: u64,
	flags: u64,
	sp: u64,
	ss: u64,
	pub fn format(
			self: IdtFrame,
			comptime fmt: []const u8,
			options: std.fmt.FormatOptions,
			writer: anytype,
		) !void {
			_ = fmt;
			_ = options;
			try writer.print("IdtFrame{{ .ip = 0x{X}, .cs = 0x{X}, .flags = 0x{X}, sp = 0x{X}, .ss = 0x{X} }}", .{
				self.ip,
				self.cs,
				self.flags,
				self.sp,
				self.ss
			});
		}
};

const page_fault_ecode = packed struct {
	present: u1,
	write: u1,
	user: u1,
	reserved_bit_set: u1,
	instruction_fetch: u1,
	protection_key_violation: u1,
	shadow_stack: u1,
	reserved: u57
};

fn get_cr2() usize {
	return asm(
		\\ movq %%cr2, %[out]
		: [out] "=r"(->usize)
		:
		:
	);
}

fn send_msg_to_fault_handler(ecode: usize, param1: usize) void {
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
	const meself = sch.current_process.?;
	if(meself.fault_port == -1) {
		sch.exit(meself);
	} else {
		// TODO: send the msg to the fault handler
		var msg = ipcfn.ipc_msg.ipc_message_t {
			.dest = meself.fault_port,
			.value0 = ecode,
			.value1 = param1,
		};
		const s = ipcfn.ipc_send(&msg);
		if(s != ipcfn.ipc_msg.EOK) {
			sch.exit(meself);
		}
		const r = ipcfn.ipc_recv(&msg);
		if(r != ipcfn.ipc_msg.EOK) {
			sch.exit(meself);
		}
	}
}

pub fn page_fault(frame: *IdtFrame, error_code: page_fault_ecode) callconv(.Interrupt) void {
	const cr2 = get_cr2();
	std.log.err("Page fault: {}\n{}\nat address 0x{x} (IP: {x})", .{error_code, frame, cr2, frame.ip});
	//main.hcf();
	send_msg_to_fault_handler(0x14, cr2);
}

pub fn double_fault(frame: *IdtFrame, error_code: u64) callconv(.Interrupt) void {
	std.log.err("Double fault: {} {}", .{error_code, frame});
	main.hcf();
}
pub fn general_protection_fault(frame: *IdtFrame, error_code: u64) callconv(.Interrupt) void {
	std.log.err("General protection fault: {} {}\n", .{error_code, frame});
	//main.hcf();
	send_msg_to_fault_handler(0xD, error_code);
}

pub fn generic_error_code(frame: *IdtFrame, error_code: u64) callconv(.Interrupt) void {
	std.log.err("Unknown exception: {} {}\n", .{error_code, frame});
	//main.hcf();
	send_msg_to_fault_handler(255, error_code);
}

const exception_table = [_][]const u8 {
	"Division Error",
	"Debug",
	"Non-Maskable Interrupt",
	"Breakpoint",
	"Overflow",
	"Bound Range Exceeded",
	"Invalid Opcode",
	"Device Not Available",
	"Double Fault",
	"Coprocessor Segment Overrun",
	"Invalid TSS",
	"Segment Not Present",
	"Stack-Segment Fault",
	"General Protection Fault",
	"Page Fault",
	"Reserved",
	"x87 Floating-Point Exception",
	"Alignment Check",
	"Machine Check",
	"SIMD Floating-Point Exception",
	"Virtualization Exception",
	"Control Protection Exception",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Hypervisor Injection Exception",
	"VMM Communication Exception",
	"Security Exception",
	"Reserved",
};

pub fn generic_error_generate(n: u32) fn(*IdtFrame) callconv(.Interrupt) void {
	return struct {
		fn handler(frame: *IdtFrame) callconv(.Interrupt) void {
			std.log.err("Unknown exception #{} ({s}): {}\n", .{ n, exception_table[n], frame });
			//main.hcf();
			send_msg_to_fault_handler(n, 0);
		}
	}.handler;
}

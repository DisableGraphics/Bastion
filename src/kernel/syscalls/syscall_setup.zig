const assm = @import("../arch/x86_64/asm.zig");
const std = @import("std");
const gdt = @import("../memory/gdt.zig");

extern fn syscall_handler() callconv(.C) void;

pub fn setup_syscalls() void {
	const star: u64 = (@as(u64, 0x0B) << 48) | (@as(u64, 0x08) << 32);
	assm.write_msr(0xC0000080, assm.read_msr(0xC0000080) | 1); // enable syscall
	assm.write_msr(0xC0000082, @intFromPtr(&syscall_handler));
	assm.write_msr(0xC0000081, star);
}

pub export fn syscall_handler_stage_1(
	syscall_no: u64,
	arg0: u64,
	arg1: u64,
	arg2: u64,
	arg3: u64,
	arg4: u64,
	arg5: u64,
) callconv(.C) void {
	std.log.info("sys: {} {} {} {} {} {} {}", .{syscall_no, arg0, arg1, arg2, arg3, arg4, arg5});
}
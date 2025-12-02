const assm = @import("../arch/x86_64/asm.zig");
const std = @import("std");
const gdt = @import("../memory/gdt.zig");

const IA32_EFER = 0xC0000080;
const IA32_STAR = 0xC0000081;
const IA32_LSTAR = 0xC0000082;
const IA32_SFMASK = 0xC0000084;

extern fn syscall_handler() callconv(.C) void;

pub fn setup_syscalls() void {
	const star: u64 = (16 << 48);
	
	assm.write_msr(IA32_LSTAR, 0x200); // Disable interrupts while syscall is handled
	assm.write_msr(IA32_SFMASK, @intFromPtr(&syscall_handler));
	assm.write_msr(IA32_STAR, star); // Save code segments
	assm.write_msr(IA32_EFER, assm.read_msr(IA32_EFER) | 1); // enable syscall
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
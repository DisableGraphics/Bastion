const assm = @import("../arch/x86_64/asm.zig");
const std = @import("std");
const gdt = @import("../memory/gdt.zig");
const ipcfn = @import("../ipc/ipcfn.zig");

const IA32_EFER = 0xC0000080;
const IA32_STAR = 0xC0000081;
const IA32_LSTAR = 0xC0000082;
const IA32_SFMASK = 0xC0000084;

extern fn syscall_handler() callconv(.C) void;

pub fn setup_syscalls() void {
	const star: u64 = (0x13 << 48) | (0x8 << 32);
	
	assm.write_msr(IA32_SFMASK, 0x200); // Disable interrupts while syscall is handled
	assm.write_msr(IA32_LSTAR, @intFromPtr(&syscall_handler));
	assm.write_msr(IA32_STAR, star); // Save code segments
	assm.write_msr(IA32_EFER, assm.read_msr(IA32_EFER) | 1); // enable syscall
}

pub export fn syscall_handler_stage_1(
	rdi: u64,
	rsi: u64,
	rdx: u64,
	r10: u64,
	r8: u64,
	r9: u64,
	rax: u64,
) callconv(.C) u64 {
	std.log.info("sys: {} {} {} {} {} {} {}", .{rax, rdi, rsi, rdx, r10, r8, r9});
	switch(rax) {
		// ipc_send
		std.math.maxInt(u64) => {
			return @intCast(ipcfn.ipc_send(@ptrFromInt(rdi)));
		},
		// ipc_recv
		std.math.maxInt(u64) - 1 => {
			return @intCast(ipcfn.ipc_recv(@ptrFromInt(rdi)));
		},
		// any other: send msg to configured syscall server (TODO)
		else => {
			return std.math.maxInt(u64);
		}
	}
}
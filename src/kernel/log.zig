const serial = @import("serial/serial.zig");
const spin = @import("sync/ispinlock.zig");
const std = @import("std");
const idt = @import("interrupts/idt.zig");
const assm = @import("arch/x86_64/asm.zig");

var writer = serial.Serial.writer();
var lock = spin.ISpinLock.init();

pub fn logfn(comptime level: std.log.Level,
    comptime scope: @Type(.enum_literal),
    comptime format: []const u8,
    args: anytype,
) void  {
	const scope_prefix = "(" ++ @tagName(scope) ++ "): ";
	const prefix = "[" ++ comptime level.asText() ++ "] " ++ switch(scope) {
		.default => "",
		else => scope_prefix
	};
	const flags = lock.lock();
	defer lock.unlock(flags);
	writer.print(prefix ++ format ++ "\n", args) catch return;
}
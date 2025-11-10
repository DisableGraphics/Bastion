const serial = @import("serial/serial.zig");
const spin = @import("sync/spinlock.zig");
const std = @import("std");
const idt = @import("interrupts/idt.zig");

var writer = serial.Serial.writer();
var lock = spin.SpinLock.init();

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
	idt.disable_interrupts();
	lock.lock();
	
	defer idt.enable_interrupts();
	defer lock.unlock();
	writer.print(prefix ++ format ++ "\n", args) catch return;
}
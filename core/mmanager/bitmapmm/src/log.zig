const debug = @cImport(
	@cInclude("debug.h")
);

const std = @import("std");

extern fn putc(c: c_int) void;

pub const PutcWriter = struct {
	pub const Error = error{WriteFailed};

	pub fn write(_: *PutcWriter, bytes: []const u8) Error!usize {
		for (bytes) |b| {
			putc(@intCast(b));
		}
		return bytes.len;
	}

	pub fn writer(self: *PutcWriter) std.io.Writer(*PutcWriter, Error, write) {
		return .{ .context = self };
	}
};

pub fn logfn(comptime level: std.log.Level,
    comptime scope: @Type(.enum_literal),
    comptime format: []const u8,
    args: anytype,
) void  {
	const scope_prefix = "(BITMAPMM) (" ++ @tagName(scope) ++ "): ";
	const prefix = "[" ++ comptime level.asText() ++ "] " ++ switch(scope) {
		.default => "",
		else => scope_prefix
	};
	var ctx = PutcWriter{};
	const writer = ctx.writer();
	writer.print(prefix ++ format ++ "\n", args) catch return;
}

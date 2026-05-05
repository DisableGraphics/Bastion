const std = @import("std");

pub const RegFlags = enum {
	WITH_INTERRUPTS,
	WITHOUT_INTERRUPTS
};

pub const registers = extern struct {
	r15: u64,
	r14: u64,
	r13: u64,
	r12: u64,
	rbp: u64,
	rbx: u64,
	rflags: u64,
	pub fn format(
		self: @This(),
		comptime fmt: []const u8,
		options: std.fmt.FormatOptions,
		writer: anytype,
	) !void {
		_ = fmt;
		_ = options;
		try writer.print("{{ r15 = {x}, r14 = {x}, r13 = {x}, r12 = {x}, rbp = {x}, rbx = {x}, rflags = {x}}}", .{
			self.r15,
			self.r14,
			self.r13,
			self.r12,
			self.rbp,
			self.rbx,
			self.rflags
		});
	}
	pub fn init(rflags: RegFlags) registers {
		const flags: u64 = switch (rflags) {
			.WITH_INTERRUPTS => 0x202,
			.WITHOUT_INTERRUPTS => 0x2
		};
		return registers{
			.r15 = 0,
			.r14 = 0,
			.r13 = 0,
			.r12 = 0,
			.rbp = 0,
			.rbx = 0,
			.rflags = flags
		};
	}
};
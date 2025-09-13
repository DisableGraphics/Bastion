// Based on https://github.com/ThatDreche/spinlock/blob/main/spinlock.zig

const std = @import("std");
const idt = @import("../interrupts/idt.zig");

const spinlock_state = enum(u8){Unlocked,Locked};

pub const SpinLock = struct {
	
	state: std.atomic.Value(spinlock_state),

	pub fn init() SpinLock {
		return .{.state = std.atomic.Value(spinlock_state).init(.Unlocked)};
	}

	pub fn init_locked() SpinLock {
		return .{.state = std.atomic.Value(spinlock_state).init(.Locked)};
	}

	pub fn lock(self: *SpinLock) void {
		while (!self.try_lock()) {
			std.atomic.spinLoopHint();
		}
	}

	pub fn try_lock(self: *SpinLock) bool {
		return switch (self.state.swap(.Locked, .acquire)) {
			.Locked => false,
			.Unlocked => true,
		};
	}

	pub fn unlock(self: *SpinLock) void {
		self.state.store(.Unlocked, .release);
	}
};
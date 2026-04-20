const std = @import("std");
const spinlock_state = enum(u8){Unlocked,Locked};

pub const SpinLock = struct {
	state: std.atomic.Value(spinlock_state),

	pub fn init() SpinLock {
		return .{.state = std.atomic.Value(spinlock_state).init(.Unlocked)};
	}

	pub fn lock(self: *SpinLock) void {
		while (!self.try_lock()) {
			std.atomic.spinLoopHint();
		}
	}

	pub inline fn try_lock(self: *SpinLock) bool {
		return switch (self.state.swap(.Locked, .acquire)) {
			.Locked => false,
			.Unlocked => true,
		};
	}

	pub fn unlock(self: *SpinLock) void {
		self.state.store(.Unlocked, .release);
	}
};
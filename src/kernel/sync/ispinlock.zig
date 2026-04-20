const spin = @import("spinlock.zig");
const assm = @import("../arch/x86_64/asm.zig");

// Interrupt-safe spinlock
pub const ISpinLock = struct {
	lck: spin.SpinLock,
	pub fn init() @This() {
		return .{
			.lck = spin.SpinLock.init()
		};
	}

	pub fn lock(self: *@This()) usize {
		const flags = assm.irqdisable();
		self.lck.lock();
		return flags;
	}

	pub fn unlock(self: *@This(), flags: usize) void {
		self.lck.unlock();
		assm.irqrestore(flags);
	}
};
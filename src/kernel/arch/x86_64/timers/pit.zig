const assm = @import("../asm.zig");
const std = @import("std");
const PIT_FREQUENCY = 1193182;
const PIT_CHANNEL0 = 0x40;
const PIT_COMMAND = 0x43;

pub const PIT = struct {
	micros_per_tick: i32,
	countdown: std.atomic.Value(i32),
	pub fn init() PIT {
		const micros = 1000;
		const result = (1_193_182 * micros)/1_000_000;
		const command = 0x34;
		const div: u16 = @truncate(result);
		assm.outb(PIT_COMMAND, command);
		assm.outb(PIT_CHANNEL0, @as(u8,(div & 0xFF)));
		assm.outb(PIT_CHANNEL0, @as(u8, ((div >> 8) & 0xFF)));
		return .{
			.micros_per_tick = micros,
			.countdown = std.atomic.Value(i32).init(0)
		};
	}
	pub fn read_counter() u16 {
		assm.outb(PIT_COMMAND, 0x00);
		const low = assm.inb(PIT_CHANNEL0);
		const high = assm.inb(PIT_CHANNEL0);
		return (@as(u16, high) << 8) | low;
	}

	pub fn sleep(self: *volatile PIT, ms: i32) void {
		const countdown: *std.atomic.Value(i32) = @volatileCast(&self.countdown);
		countdown.store(ms * 1000, .release);
		while(countdown.load(.acquire) > 0) {
			asm volatile("hlt");
		}
	}

	pub fn disable(self: *const PIT) void {
		_ = self;
		assm.outb(PIT_COMMAND, 0x30);

		assm.outb(PIT_CHANNEL0, 0xFF);
		assm.outb(PIT_CHANNEL0, 0xFF);
	}

	pub fn on_irq(s: ?*volatile anyopaque) void {
		var self: *volatile PIT = @ptrCast(@alignCast(s.?));
		var countdown: *std.atomic.Value(i32) = @volatileCast(&self.countdown);
		_ = countdown.fetchSub(self.micros_per_tick, .acq_rel);
	}
};
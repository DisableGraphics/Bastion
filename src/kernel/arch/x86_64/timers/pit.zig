const assm = @import("../asm.zig");
const PIT_FREQUENCY = 1193182;
const PIT_CHANNEL0 = 0x40;
const PIT_COMMAND = 0x43;

pub const PIT = struct {
	millis_per_tick: i32,
	countdown: i32,
	pub fn init() PIT {
		const command = 0x34;
		const div: u16 = @truncate((1193182 * 100_000)/1000000); // 100_000us -> 100 ms
		assm.outb(PIT_COMMAND, command);
		assm.outb(PIT_CHANNEL0, @as(u8,(div & 0xFF)));
		assm.outb(PIT_CHANNEL0, @as(u8, ((div >> 8) & 0xFF)));
		return .{
			.millis_per_tick = 100,
			.countdown = 0
		};
	}
	pub fn read_counter() u16 {
		assm.outb(PIT_COMMAND, 0x00);
		const low = assm.inb(PIT_CHANNEL0);
		const high = assm.inb(PIT_CHANNEL0);
		return (@as(u16, high) << 8) | low;
	}

	pub fn sleep(self: *volatile PIT, ms: i32) void {
		self.countdown = ms;
		while(self.countdown > 0) {
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
		self.countdown -= self.millis_per_tick;
	}
};
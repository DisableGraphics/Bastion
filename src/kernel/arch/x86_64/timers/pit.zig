const assm = @import("../asm.zig");
const std = @import("std");
const PIT_FREQUENCY = 1193182;
const PIT_CHANNEL0 = 0x40;
const PIT_COMMAND = 0x43;

pub const PIT = struct {
	var micros_per_tick: i32 = 0;
	var countdown: std.atomic.Value(i32) = std.atomic.Value(i32).init(0);
	pub fn init() void {
		const micros = 1000;
		const result = (1_193_182 * micros)/1_000_000;
		const command = 0x34;
		const div: u16 = @truncate(result);
		assm.outb(PIT_COMMAND, command);
		assm.outb(PIT_CHANNEL0, @as(u8,(div & 0xFF)));
		assm.outb(PIT_CHANNEL0, @as(u8, ((div >> 8) & 0xFF)));
		micros_per_tick = micros;
	}
	pub fn read_counter() u16 {
		assm.outb(PIT_COMMAND, 0x00);
		const low = assm.inb(PIT_CHANNEL0);
		const high = assm.inb(PIT_CHANNEL0);
		return (@as(u16, high) << 8) | low;
	}

	pub fn sleep(ms: i32) void {
		countdown.store(ms * 1000, .seq_cst);
		while(countdown.load(.seq_cst) > 0) {
			asm volatile("hlt");
		}
	}

	pub fn disable() void {
		assm.outb(PIT_COMMAND, 0x30);

		assm.outb(PIT_CHANNEL0, 0xFF);
		assm.outb(PIT_CHANNEL0, 0xFF);
	}

	pub fn on_irq(_: ?*volatile anyopaque) void {
		_ = countdown.fetchSub(micros_per_tick, .seq_cst);
	}
};
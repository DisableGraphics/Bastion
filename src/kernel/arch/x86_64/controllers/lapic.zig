const assm = @import("../asm.zig");
const std = @import("std");
const pitt = @import("../timers/pit.zig");
const cpu = @import("../cpu_status.zig");

const IA32_APIC_BASE_MSR = 0x1B;
const IA32_APIC_BASE_MSR_ENABLE = 0x800;
const LAPIC_DM_EXTINT = (0x7 << 8);

pub fn lapic_physaddr() u64 {
	const b = assm.read_msr(IA32_APIC_BASE_MSR);
	return b & 0xfffff000;
}

pub const LAPIC = struct {
	lapic_base: usize,
	timer_event: ?*const fn(?*anyopaque) void,
	arg: ?*anyopaque,
	pub fn init(lapic_table_virtaddr: usize, lapic_base_physaddr: usize, is_bsp: bool) LAPIC {
		var self: LAPIC = .{
			.lapic_base = lapic_table_virtaddr,
			.timer_event = null,
			.arg = null,
		};
		self.set_apic_base(lapic_base_physaddr);

		// enable
		self.write_reg(0xF0, self.read_reg(0xF0) | 0x100);
		if(is_bsp) {
			self.pic_passthrough();
		} else {
			self.write_reg(0x350, self.read_reg(0x350) | (1 << 16));
		}

		return self;
	}

	pub fn init_timer(self: *LAPIC, freqms: i32, pit: *pitt.PIT) void {
		self.write_reg(0x3E0, 0x3);
		self.write_reg(0x380, 0xFFFFFFFF);
	
		pit.sleep(freqms);
	
		self.write_reg(0x320, (1 << 16));
		const ticks: u32 = 0xFFFFFFFF - self.read_reg(0x390);
		self.write_reg(0x320, 0x20 | (0b01 << 17));
		self.write_reg(0x3E0, 0x3);
		self.write_reg(0x380, ticks);
		std.log.info("ticks in one second: {}", .{ticks});
	}

	fn pic_passthrough(self: *volatile LAPIC) void {
		var lint0 = self.read_reg(0x350);
		lint0 &= ~@as(u32, (1 << 16));
		lint0 &= ~@as(u32, (7 << 8));
		lint0 |= LAPIC_DM_EXTINT;
		self.write_reg(0x350, lint0);
	}

	fn write_reg(self: *volatile LAPIC, reg: usize, value: u32) void {
		const rg = self.lapic_reg(reg);
		rg.* = value;
	}

	fn read_reg(self: *volatile LAPIC, reg: usize) u32 {
		const rg = self.lapic_reg(reg);
		return rg.*;
	}

	fn lapic_reg(self: *volatile LAPIC, offset: usize) *volatile u32 {
		return @ptrFromInt(self.lapic_base + offset);
	}
	fn set_apic_base(self: *const LAPIC, base: usize) void {
		_ = self;
		assm.write_msr(IA32_APIC_BASE_MSR, (base & 0xfffff0000) | IA32_APIC_BASE_MSR_ENABLE);
	}

	pub fn set_on_timer(self: *LAPIC, timer_event: ?*const fn(?*anyopaque) void, arg: ?*anyopaque)void {
		self.timer_event = timer_event;
		self.arg = arg;
	}

	pub fn get_cpuid(lapic_table: usize) u64 {
		const id_reg: *volatile u32 = @ptrFromInt(lapic_table + 0x20);
		return @as(u64, id_reg.*);
	}

	pub fn on_irq(s: ?*volatile anyopaque) void {
		const self: *volatile LAPIC = @ptrCast(@alignCast(s.?));
		self.write_reg(0xB0, 0); // EOI
		if(self.timer_event) |ev| {
			ev(self.arg);
		}
	}
};
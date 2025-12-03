const assm = @import("../asm.zig");
const std = @import("std");
const idt = @import("../../../interrupts/idt.zig");
const handlers = @import("../../../interrupts/handlers.zig");
const porttable = @import("../../../interrupts/iporttable.zig");

const PIC1_PORT = 0x20;
const PIC2_PORT = 0xA0;
const PIC1_COMMAND = PIC1_PORT;
const PIC1_DATA = PIC1_PORT + 1;
const PIC2_COMMAND = PIC2_PORT;
const PIC2_DATA = PIC2_PORT + 1;

const ICW1_ICW4	= 0x01;
const ICW1_SINGLE =		0x02;
const ICW1_INTERVAL4 =	0x04;
const ICW1_LEVEL =		0x08;
const ICW1_INIT =		0x10;

const ICW4_8086 =		0x01;
const ICW4_AUTO =		0x02;
const ICW4_BUF_SLAVE =	0x08;
const ICW4_BUF_MASTER =	0x0C;
const ICW4_SFNM =		0x10;

const CASCADE_IRQ = 2;
const PIC_READ_IRR = 0x0a;
const PIC_READ_ISR = 0x0b;
const PIC_EOI = 0x20;	

fn makeIRQ(comptime n: usize) type {
    return struct {
        pub fn func(_: *handlers.IdtFrame) callconv(.Interrupt) void {
			// Handle spurious interrupts
			if(n == 7) {
				const isr = PIC.get_isr();
				if(isr & (1 << 7) == 0) {
					return; // Just forget about it. No need to send eoi
				}
			} else if(n == 15) {
				const isr = PIC.get_isr();
				if(isr & (1 << 15) == 0) {
					// This is a bit funnier because I have to send EOI to the 1st PIC
					// Note: I put 2 because I can put any number there so I chose 2
					PIC.eoi(2);
					return; 
				}
			}
			PIC.fn_table[n](PIC.arg_table[n]);
			if(n != 16) {
				porttable.InterruptPortTable.notify_interrupt(n);
			}
			if(n < 16) {
				PIC.eoi(n);
			}
        }
		
    };
}

const nirqs = 32;

pub const IRQs = blk: {
    var funcs: [nirqs]*const fn(*handlers.IdtFrame) callconv(.Interrupt) void = undefined;
    for (0..nirqs) |i| {
        funcs[i] = makeIRQ(i).func;
    }
    break :blk funcs;
};

fn stub(irq: PIC.arg_type) void {
	std.log.debug("No IRQ registered for #{any}", .{irq});
}

pub const PIC = struct {
	const arg_type = ?*volatile anyopaque;
	const pic_func = fn(arg_type) void;
	var fn_table: [nirqs]*const pic_func = undefined;
	var arg_table: [nirqs]?*volatile anyopaque = undefined;
	
	const PIC1_MAP = 0x20;
	const PIC2_MAP = 0x28;
	pub fn init() PIC {
		var self = PIC{};
		self.remap(PIC1_MAP, PIC2_MAP);
		init_irq_fns();
		return self;
	}

	fn init_irq_fns() void {
		for(0..nirqs) |i| {
			fn_table[i] = &stub;
			const vector = i + PIC1_MAP;
			idt.insert_idt_entry(vector, IRQs[i]);
		}
	}

	pub fn set_irq_handler(self: *PIC, irq: u5, argument: arg_type, function: *const pic_func) void {
		_ = self;
		fn_table[irq] = function;
		arg_table[irq] = argument;
	}

	pub fn remap(self: *PIC, vector1: u8, vector2: u8) void {
		_ = self;
		assm.outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
		assm.io_wait();
		assm.outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
		assm.io_wait();
		assm.outb(PIC1_DATA, vector1);
		assm.io_wait();
		assm.outb(PIC2_DATA, vector2);
		assm.io_wait();
		assm.outb(PIC1_DATA, 1 << CASCADE_IRQ);
		assm.io_wait();
		assm.outb(PIC2_DATA, 2);
		assm.io_wait();
		assm.outb(PIC1_DATA, ICW4_8086);
		assm.io_wait();
		assm.outb(PIC2_DATA, ICW4_8086);
		assm.io_wait();

		assm.outb(PIC1_DATA, 0);
		assm.outb(PIC2_DATA, 0);
	}

	pub fn disable(self: *PIC) void {
		_ = self;

		assm.outb(PIC1_DATA, 0xFF);
		assm.outb(PIC2_DATA, 0xFF);
	}

	pub fn disable_irq(irqno: u4) void {
		const port: u16 = if(irqno < 8) PIC1_DATA else PIC2_DATA;
		const irqline: u3 = @truncate(irqno);
		const value = assm.inb(port) | (@as(u8, 1) << irqline);
		assm.outb(port, value);
	}

	pub fn enable_irq(irqno: u4) void {
		const port: u16 = if(irqno < 8) PIC1_DATA else PIC2_DATA;
		const irqline: u3 = @truncate(irqno);
		const value = assm.inb(port) & ~(@as(u8, 1) << irqline);
		assm.outb(port, value);
	}

	fn get_int_reg(ocw3: u8) u16 {
		assm.outb(PIC1_COMMAND, ocw3);
		assm.outb(PIC2_COMMAND, ocw3);
		return (@as(u16, assm.inb(PIC2_COMMAND)) << 8) | assm.inb(PIC1_COMMAND);
	}

	pub fn get_irr() u16 {
		return get_int_reg(PIC_READ_IRR);
	}

	pub fn get_isr() u16 {
		return get_int_reg(PIC_READ_ISR);
	}

	pub fn eoi(irq: u16) void {
		if(irq >= 8)
			assm.outb(PIC2_COMMAND,PIC_EOI);
		assm.outb(PIC1_COMMAND,PIC_EOI);
	}
};
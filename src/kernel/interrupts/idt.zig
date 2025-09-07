const handlers = @import("handlers.zig");
const main = @import("../main.zig");
const std = @import("std");

pub const idt_entry_t = packed struct { 
	isr_low: u16, 
	segment_selector: u16, 
	ist: u8,
	attributes: u8,
	isr_mid: u16,
	isr_high: u32, 
	reserved: u32
};

pub const idtr_t = packed struct { idt_limit: u16, idt_base_address: u64 };

var idtr: idtr_t = undefined;
const IDT_MAX_DESCRIPTORS = 256;
var idt_global: [IDT_MAX_DESCRIPTORS]idt_entry_t align(0x10) = undefined;

fn load_idtr(idtr_ptr: *idtr_t) void {
	asm volatile ("lidt (%[idtr_ptr])"
		:
		: [idtr_ptr] "r" (idtr_ptr)
		: "memory"
	);
}

fn to_idt_entry(func: *const anyopaque, flags: u8) idt_entry_t {
	const func_addr: usize = @intFromPtr(func);
	const idte: idt_entry_t = .{
		.isr_low = @truncate(func_addr & 0xFFFF),
		.segment_selector = 0x8,
		.ist = 0,
		.attributes = flags,
		.isr_mid = @truncate((func_addr >> 16) & 0xFFFF),
		.isr_high = @truncate((func_addr >> 32) & 0xFFFFFFFF),
		.reserved = 0
	};
	return idte;
}

pub fn insert_idt_entry(vector: usize, fnptr: *const fn(frame: *handlers.IdtFrame) callconv(.Interrupt) void) void {
	idt_global[vector] = to_idt_entry(@ptrCast(fnptr), 0x8E);
}

var idt_is_setup = std.atomic.Value(bool).init(false);

fn setup_idt(idt: *[256]idt_entry_t) void {
	idt_is_setup.store(true, .release);
	for(0..8) |i| {
		idt[i] = to_idt_entry(@ptrCast(&handlers.generic_error), 0x8E);
	}
	idt[8] = to_idt_entry(@ptrCast(&handlers.double_fault), 0x8E);

	idt[9] = to_idt_entry(@ptrCast(&handlers.generic_error), 0x8E);
	for(10..15) |i| {
		if(i != 13 and i != 14) {
			idt[i] = to_idt_entry(@ptrCast(&handlers.generic_error_code), 0x8E);
		}
	}

	idt[13] = to_idt_entry(@ptrCast(&handlers.general_protection_fault), 0x8E);
	idt[14] = to_idt_entry(@ptrCast(&handlers.page_fault), 0x8E);

	idt[15] = to_idt_entry(@ptrCast(&handlers.generic_error), 0x8E);
	idt[16] = to_idt_entry(@ptrCast(&handlers.generic_error), 0x8E);
	idt[17] = to_idt_entry(@ptrCast(&handlers.generic_error_code), 0x8E);
	idt[18] = to_idt_entry(@ptrCast(&handlers.generic_error), 0x8E);
	idt[19] = to_idt_entry(@ptrCast(&handlers.generic_error), 0x8E);
	idt[20] = to_idt_entry(@ptrCast(&handlers.generic_error), 0x8E);
	idt[21] = to_idt_entry(@ptrCast(&handlers.generic_error_code), 0x8E);
	for(20..29) |i| {
		idt[i] = to_idt_entry(@ptrCast(&handlers.generic_error), 0x8E);
	}
	idt[29] = to_idt_entry(@ptrCast(&handlers.generic_error_code), 0x8E);
	idt[30] = to_idt_entry(@ptrCast(&handlers.generic_error_code), 0x8E);
	idt[31] = to_idt_entry(@ptrCast(&handlers.generic_error), 0x8E);
}

pub fn enable_interrupts() void {
	asm volatile("sti");
}

pub fn disable_interrupts() void {
	asm volatile("cli");
}

var eints = false;

pub fn can_enable_interrupts() bool {
	if(@inComptime()) return false;
	return eints;
}

pub fn set_enable_interrupts() void {
	eints = true;
}

pub fn init() void {
	if(!idt_is_setup.load(.acquire)) {
		setup_idt(&idt_global);
		idtr.idt_base_address = @intFromPtr(&idt_global);
		idtr.idt_limit = @sizeOf(@TypeOf(idt_global)) - 1;
	}
	load_idtr(&idtr);
}

const serial = @import("../serial/serial.zig");
const main = @import("../main.zig");
const std = @import("std");
const spin = @import("../sync/spinlock.zig");

pub const IdtFrame = extern struct {
    ip: u64,
    cs: u64,
    flags: u64,
    sp: u64,
    ss: u64,
	pub fn format(
            self: IdtFrame,
            comptime fmt: []const u8,
            options: std.fmt.FormatOptions,
            writer: anytype,
        ) !void {
            _ = fmt;
            _ = options;
            try writer.print("IdtFrame{{ .ip = 0x{X}, .cs = 0x{X}, .flags = 0x{X}, sp = 0x{X}, .ss = 0x{X} }}", .{
                self.ip,
                self.cs,
				self.flags,
				self.sp,
				self.ss
            });
        }
};

const page_fault_ecode = packed struct {
	present: u1,
	write: u1,
	user: u1,
	reserved_bit_set: u1,
	instruction_fetch: u1,
	protection_key_violation: u1,
	shadow_stack: u1,
	reserved: u57
};

fn get_cr2() usize {
	return asm(
		\\ movq %%cr2, %[out]
		: [out] "=r"(->usize)
		:
		:
	);
}

pub fn page_fault(frame: *IdtFrame, error_code: page_fault_ecode) callconv(.Interrupt) void {
	var writer = serial.Serial.writer();
	const cr2 = get_cr2();
	_ = writer.print("Page fault: {}\n{}\nat address 0x{x} (IP: {x})", .{error_code, frame, cr2, frame.ip}) catch @panic("Could not write in page_fault handler");
	main.hcf();
}

pub fn double_fault(frame: *IdtFrame, error_code: u64) callconv(.Interrupt) void {
	var writer = serial.Serial.writer();
	_ = writer.print("Double fault: {} {}", .{error_code, frame}) catch @panic("Could not write in double_fault handler");
	main.hcf();
}
var spinlock = spin.SpinLock.init();
pub fn general_protection_fault(frame: *IdtFrame, error_code: u64) callconv(.Interrupt) void {
	spinlock.lock();
	var writer = serial.Serial.writer();
	_ = writer.print("General protection fault: {} {}\n", .{error_code, frame}) 
		catch @panic("Could not write in general_protection_fault handler");
	spinlock.unlock();
	main.hcf();
}

pub fn generic_error_code(frame: *IdtFrame, error_code: u64) callconv(.Interrupt) void {
	var writer = serial.Serial.writer();
	_ = writer.print("Unknown exception: {} {}\n", .{error_code, frame}) 
		catch @panic("Could not write in generic_error_code handler");
	main.hcf();
}

pub fn generic_error_generate(n: u32) fn(*IdtFrame) callconv(.Interrupt) void {
    return struct {
        fn handler(frame: *IdtFrame) callconv(.Interrupt) void {
            var writer = serial.Serial.writer();
            _ = writer.print("Unknown exception #{}: {}\n", .{ n, frame })
                catch @panic("Could not write in generic_error_generate handler");
            main.hcf();
        }
    }.handler;
}

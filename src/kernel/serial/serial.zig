const inlineasm = @import("../arch/x86_64/asm.zig");
const std = @import("std");
pub const PORT = 0x3f8;

pub const SerialError = error {
	FAULTY_SERIAL
};

const writeerr = error{EOF};

pub const Serial = struct {
	pub fn init() !void {
		inlineasm.outb(PORT + 1, 0x00);    // Disable all interrupts
		inlineasm.outb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
		inlineasm.outb(PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
		inlineasm.outb(PORT + 1, 0x00);    //                  (hi byte)
		inlineasm.outb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
		inlineasm.outb(PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
		inlineasm.outb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
		inlineasm.outb(PORT + 4, 0x1E);    // Set in loopback mode, test the serial chip
		inlineasm.outb(PORT + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

		// Check if serial is faulty (i.e: not same byte as sent)
		if(inlineasm.inb(PORT + 0) != 0xAE) {
			return SerialError.FAULTY_SERIAL;
		}

		// If serial is not faulty set it in normal operation mode
		// (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
		inlineasm.outb(PORT + 4, 0x0F);
	}

	fn serial_received() u8 {
		return inlineasm.inb(PORT + 5) & 1;
	}

	pub fn read() u8 {
		while (serial_received() == 0){}

   		return inlineasm.inb(PORT);
	}

	fn is_transmit_empty() u8 {
		return inlineasm.inb(PORT + 5) & 0x20;
	}

	pub fn write(char: u8) void {
		while (is_transmit_empty() == 0){}

   		inlineasm.outb(PORT, char);
	}

	pub fn write_str(str: []const u8) void {
		for(str) |char| {
			write(char);
		}
	}

	fn write_wr(_: *Serial, data: []const u8) error{OutOfMemory}!usize {
		write_str(data);
        return data.len;
    }

    pub const SerialWriter = std.io.Writer(*Serial, error{OutOfMemory}, write_wr);

    pub fn writer() SerialWriter {
        return .{.context = undefined};
    }

};
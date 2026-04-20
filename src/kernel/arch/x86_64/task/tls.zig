const assm = @import("../asm.zig");

const fsbase = 0xC0000100;
const gsbase = 0xC0000101;

pub const tls_registers = extern struct {
	fs: u64 = 0,
	gs: u64 = 0,

	pub fn init(value1: u64, value2: u64) tls_registers {
		return .{
			.fs = value1,
			.gs = value2
		};
	}
	// Save registers from the CPU to this structure
	pub fn save(self: *@This()) void {
		self.fs = assm.read_msr(fsbase);
		self.gs = assm.read_msr(gsbase);
	}

	// Load the TLS registers to the CPU from this structure 
	pub fn load(self: *@This()) void {
		assm.write_msr(fsbase, self.fs);
		assm.write_msr(gsbase, self.gs);
	}
};
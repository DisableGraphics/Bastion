const handlers = @import("handlers.zig");
const main = @import("../main.zig");
const std = @import("std");
const cid = @import("../arch/x86_64/cpuid.zig");

fn is_vector(ip: u64) bool {
	const instruction_ptr: *u8 = @ptrFromInt(ip);
	const val = instruction_ptr.*;
	if(val == 0x66 or val == 0xF2 or val == 0xF3) {
		const second_opcode: *u8 = @ptrFromInt(ip+1);
		return if(second_opcode.* == 0x0F) true else false;
	} else if(val == 0xC4 or val == 0xC5) {
		return true;
	} else {
		return false;
	}
}

fn enable_vector() void {
	// At least sse1 & sse2 are mandatory for x86_64
	enable_sse();
}

extern fn enable_sse() callconv(.C) void;

pub fn ill_instr(frame: *handlers.IdtFrame) callconv(.Interrupt) void {
	// This one is funny: if we have an SSE/AVX/MMX/x87 instruction we need to search wether the current task we're running
	// has a buffer to load from. If it doesn't, we have to allocate a buffer for the task and load from it.
	// If it has a buffer, then just loading is enough.
	// In other case we just call the registered handler
	// and also of course to enable vector extensions.

	if(is_vector(frame.ip)) {
		std.log.info("yea is vector", .{});
		enable_vector();
	} else {
		// TODO: send message via port so that the true illegal instruction is handled
		main.hcf();
	}
}
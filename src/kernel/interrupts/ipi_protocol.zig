const std = @import("std");
const kmm = @import("../memory/kmm.zig");
const page = @import("../memory/pagemanager.zig");
const main = @import("../main.zig");

pub const IPIProtocolMessageType = enum(u8) {
	NONE,
	TASK_LOAD_BALANCING_REQUEST,
	TASK_LOAD_BALANCING_RESPONSE
};

pub const IPIProtocolPayload = extern struct {
	type: IPIProtocolMessageType,
	p0: u64,
	p1: u64,
	p2: u64,
	pub fn init() IPIProtocolPayload {
		return .{
			.type = .NONE,
			.p0 = 0,
			.p1 = 0,
			.p2 = 0
		};
	}

	pub fn reset(self: *IPIProtocolPayload) void {
		self.type = .NONE;
		self.p0 = 0;
		self.p1 = 0;
		self.p2 = 0;
	}
};

pub const IPIProtocolHandler = struct {
	var ipiprotocol_payloads: []std.atomic.Value(IPIProtocolPayload) = undefined;

	pub fn ginit(cpus: u64, alloc: *kmm.KernelMemoryManager) !void {
		const npages = ((cpus * @sizeOf(std.atomic.Value(IPIProtocolPayload))) + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
		ipiprotocol_payloads = @as([*]std.atomic.Value(IPIProtocolPayload), @ptrFromInt((try alloc.alloc_virt(npages)).?))[0..cpus];
		for(0..cpus) |i| {
			ipiprotocol_payloads[i] = std.atomic.Value(IPIProtocolPayload).init(IPIProtocolPayload.init());
		}
	}

	pub fn handle_ipi1(arg: ?*anyopaque) void {
		_ = arg;
		const mycpu = main.mycpuid();
		const ask = ipiprotocol_payloads[mycpu];
		const msg = ask.load(.acquire);
		switch(msg.type) {
			else => std.log.err("No handler for IPI payload of type: {}", .{@tagName(msg.type)})
		}
	}
};
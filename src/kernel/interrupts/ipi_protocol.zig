const std = @import("std");
const kmm = @import("../memory/kmm.zig");
const page = @import("../memory/pagemanager.zig");
const main = @import("../main.zig");
const lpman = @import("../arch/x86_64/controllers/manager.zig");
const schmn = @import("../scheduler/manager.zig");
const lpic = @import("../arch/x86_64/controllers/lapic.zig");
const load = @import("../scheduler/loadbalancer.zig");
const tsk = @import("../scheduler/task.zig");

pub const IPIProtocolMessageType = enum(u64) {
	NONE,
	SCHEDULE,
	TASK_LOAD_BALANCING_REQUEST,
	TASK_LOAD_BALANCING_RESPONSE,
};

pub const IPIProtocolPayload = struct {
	t: std.atomic.Value(IPIProtocolMessageType),
	p0: std.atomic.Value(u64),
	p1: std.atomic.Value(u64),
	p2: std.atomic.Value(u64),
	pub fn init() IPIProtocolPayload {
		return .{
			.t = std.atomic.Value(IPIProtocolMessageType).init(.NONE),
			.p0 = std.atomic.Value(u64).init(0),
			.p1 = std.atomic.Value(u64).init(0),
			.p2 = std.atomic.Value(u64).init(0),
		};
	}

	pub fn init_with_data(t: IPIProtocolMessageType, p0: u64, p1: u64, p2: u64) IPIProtocolPayload {
		return .{
			.t = std.atomic.Value(IPIProtocolMessageType).init(t),
			.p0 = std.atomic.Value(u64).init(p0),
			.p1 = std.atomic.Value(u64).init(p1),
			.p2 = std.atomic.Value(u64).init(p2),

		};
	}
};

pub const IPIProtocolHandler = struct {
	var ipiprotocol_payloads: []IPIProtocolPayload = undefined;

	pub fn ginit(cpus: u64, alloc: *kmm.KernelMemoryManager) !void {
		const npages = ((cpus * @sizeOf(IPIProtocolPayload)) + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
		ipiprotocol_payloads = @as([*]IPIProtocolPayload, @ptrFromInt((try alloc.alloc_virt(npages)).?))[0..cpus];
		for(0..cpus) |i| {
			ipiprotocol_payloads[i] = IPIProtocolPayload.init();
		}
	}

	pub fn send_ipi(destination: u32, payload: IPIProtocolPayload) void {
		ipiprotocol_payloads[destination] = payload;
		const mycpuid = main.mycpuid();
		const lapic = lpman.LAPICManager.get_lapic(mycpuid);
		lapic.send_ipi(destination);
	}

	pub fn send_ipi_broadcast(payload: IPIProtocolPayload) void {
		const mycpuid = main.mycpuid();
		const lapic = lpman.LAPICManager.get_lapic(mycpuid);
		for(0..ipiprotocol_payloads.len) |i| {
			if(i != mycpuid) {
				lapic.send_ipi(@truncate(i));
				ipiprotocol_payloads[i] = payload;
			}
		}
	}

	pub fn handle_ipi(arg: ?*volatile anyopaque) void {
		_ = arg;
		const mycpu = main.mycpuid();
		const ask = ipiprotocol_payloads[mycpu];
		const msgt = ask.t.load(.acquire);
		const p0 = ask.p0.load(.acquire);
		_ = ask.p1.load(.acquire);
		_ = ask.p2.load(.acquire);

		const lapic = lpman.LAPICManager.get_lapic(mycpu);
		switch(msgt) {
			IPIProtocolMessageType.SCHEDULE => {
				const sch = schmn.SchedulerManager.get_scheduler_for_cpu(mycpu);
				sch.on_irq_tick();
			},
			IPIProtocolMessageType.TASK_LOAD_BALANCING_REQUEST => {
				const sch = schmn.SchedulerManager.get_scheduler_for_cpu(mycpu);
				const task = load.LoadBalancer.find_task(sch);
				if(task != null and p0 < main.km.hhdm_offset) {
					IPIProtocolHandler.send_ipi(@truncate(p0), 
						IPIProtocolPayload.init_with_data(IPIProtocolMessageType.TASK_LOAD_BALANCING_RESPONSE, 
						@intFromPtr(task.?), 0,0));
				}

			},
			IPIProtocolMessageType.TASK_LOAD_BALANCING_RESPONSE => {
				const sch = schmn.SchedulerManager.get_scheduler_for_cpu(mycpu);
				if(p0 >= main.km.hhdm_offset) {
					const task: *tsk.Task = @ptrFromInt(p0);
					sch.add_task(task);
				}
			},
			else => {
				std.log.err("No handler for IPI payload of type: {s}", .{@tagName(msgt)});
			}
		}
		lapic.eoi();
	}
};
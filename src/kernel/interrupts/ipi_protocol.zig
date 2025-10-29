const std = @import("std");
const kmm = @import("../memory/kmm.zig");
const page = @import("../memory/pagemanager.zig");
const main = @import("../main.zig");
const lpman = @import("../arch/x86_64/controllers/manager.zig");
const schmn = @import("../scheduler/manager.zig");
const lpic = @import("../arch/x86_64/controllers/lapic.zig");
const load = @import("../scheduler/loadbalancer.zig");
const tsk = @import("../scheduler/task.zig");
const sa = @import("../memory/stackalloc.zig");
const ta = @import("../scheduler/taskalloc.zig");
const idt = @import("../interrupts/idt.zig");
const assm = @import("../arch/x86_64/asm.zig");
const q = @import("../datastr/ring_buffer_queue.zig");
const ba = @import("../scheduler/fpu_buffer_alloc.zig");
const ioa = @import("../memory/io_bufferalloc.zig");
const tss = @import("../memory/tss.zig");

pub const IPIProtocolMessageType = enum(u64) {
	NONE,
	SCHEDULE,
	TASK_LOAD_BALANCING_REQUEST,
	TASK_LOAD_BALANCING_RESPONSE,

	LAPIC_TIMER_SYNC_STAGE_1,
	LAPIC_TIMER_SYNC_STAGE_2,

	FREE_TASK,
	FREE_FPU_BUFFER,
	FREE_IO_BITMAP
};

pub const IPIProtocolPayload = struct {
	t: IPIProtocolMessageType,
	p0: u64,
	p1: u64,
	p2: u64,
	pub fn init() IPIProtocolPayload {
		return .{
			.t = .NONE,
			.p0 = 0,
			.p1 = 0,
			.p2 = 0,
		};
	}

	pub fn init_with_data(t: IPIProtocolMessageType, p0: u64, p1: u64, p2: u64) IPIProtocolPayload {
		return .{
			.t = t,
			.p0 = p0,
			.p1 = p1,
			.p2 = p2,
		};
	}
};

const IPIQueue = q.FixedMPSCRingBuffer(IPIProtocolPayload, 64);

pub const IPIProtocolHandler = struct {
	var ipiprotocol_payloads: []IPIQueue = undefined;

	pub fn ginit(cpus: u64, alloc: *kmm.KernelMemoryManager) !void {
		const npages = ((cpus * @sizeOf(IPIQueue)) + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
		ipiprotocol_payloads = @as([*]IPIQueue, @ptrFromInt((try alloc.alloc_virt(npages)).?))[0..cpus];
		for(0..cpus) |i| {
			ipiprotocol_payloads[i] = IPIQueue.init();
		}
	}

	pub fn send_ipi(destination: u32, payload: IPIProtocolPayload) void {
		const mask = assm.irqdisable();
		const ret = ipiprotocol_payloads[destination].push(payload);
		assm.irqrestore(mask);
		if(!ret) return;
		const mycpuid = main.mycpuid();
		const lapic = lpman.LAPICManager.get_lapic(mycpuid);
		lapic.send_ipi(destination);
	}

	pub fn send_ipi_broadcast(payload: IPIProtocolPayload) void {
		const mycpuid = main.mycpuid();
		for(0..ipiprotocol_payloads.len) |i| {
			if(i != mycpuid) {
				send_ipi(@truncate(i), payload);
			}
		}
	}

	pub fn handle_ipi(arg: ?*volatile anyopaque) void {
		_ = arg;
		const mycpu = main.mycpuid();
		const lapic = lpman.LAPICManager.get_lapic(mycpu);
		while(!ipiprotocol_payloads[mycpu].is_empty()) {
			const ask = ipiprotocol_payloads[mycpu].pop();
			if(ask == null) break; // It's empty
			const ms = ask.?;
			const msgt = ms.t;
			const p0 = ms.p0;
			_ = ms.p1;
			_ = ms.p2;
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
				IPIProtocolMessageType.LAPIC_TIMER_SYNC_STAGE_1 => {
					// Just swallow the error
				},
				IPIProtocolMessageType.LAPIC_TIMER_SYNC_STAGE_2 => {
					// Just swallow the error
				},
				IPIProtocolMessageType.FREE_TASK => {
					const task: *tsk.Task = @ptrFromInt(p0);
					
					sa.KernelStackAllocator.free(task.kernel_stack) catch |err| std.log.err("Error while freeing kernel stack: {}", .{err});
					ta.TaskAllocator.free(task) catch |err| std.log.err("Error while freeing task: {}", .{err});
				},
				IPIProtocolMessageType.FREE_FPU_BUFFER => {
					const buffer: *ba.fpu_buffer = @ptrFromInt(p0);
					ba.FPUBufferAllocator.free(buffer) catch |err| std.log.err("Error while freeing FPU buffer: {}", .{err});
				},
				IPIProtocolMessageType.FREE_IO_BITMAP => {
					const buffer: *tss.io_bitmap_t = @ptrFromInt(p0);
					ioa.IOBufferAllocator.free(buffer) catch |err| std.log.err("Error while freeing IO bitmap: {}", .{err});
				},
				else => {
					std.log.err("No handler for IPI payload of type: {s}", .{@tagName(msgt)});
				}
			}
		}
		lapic.eoi();
	}
};
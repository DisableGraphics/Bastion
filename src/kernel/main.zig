const serial = @import("serial/serial.zig");
const gdt = @import("memory/gdt.zig");
const idt = @import("interrupts/idt.zig");
const allocator = @import("memory/slaballoc.zig");
const requests = @import("arch/x86_64/boot/limine.zig");
const std = @import("std");
const spin = @import("sync/spinlock.zig");
const limine = @import("limine");
const pagemanager = @import("memory/pagemanager.zig");
const log = @import("log.zig");
const framemanager = @import("memory/physicalalloc.zig");
const kmm = @import("memory/kmm.zig");
const acpi = @import("arch/x86_64/acpi/acpimanager.zig");
const pic = @import("arch/x86_64/controllers/pic.zig");
const lapic = @import("arch/x86_64/controllers/lapic.zig");
const pit = @import("arch/x86_64/timers/pit.zig");
const tsk  = @import("scheduler/task.zig");
const sch  = @import("scheduler/scheduler.zig");
const assm = @import("arch/x86_64/asm.zig");
const tss = @import("memory/tss.zig");
const schman = @import("scheduler/manager.zig");
const lpicmn = @import("arch/x86_64/controllers/manager.zig");
const tas = @import("scheduler/task.zig");
const ta = @import("scheduler/timeralloc.zig");
const handlers = @import("interrupts/handlers.zig");
const lb = @import("scheduler/loadbalancer.zig");
const ipi = @import("interrupts/ipi_protocol.zig");
const talloc = @import("scheduler/taskalloc.zig");
const sa = @import("memory/stackalloc.zig");
const fpu = @import("scheduler/fpu_buffer_alloc.zig");
const nm = @import("interrupts/illegal_instruction.zig");
const ioa = @import("memory/io_bufferalloc.zig");
const tasadd = @import("scheduler/task_adder.zig");
const pta = @import("memory/pagetablealloc.zig");
const pa = @import("ipc/portalloc.zig");
const pca = @import("ipc/portchunkalloc.zig");
const sysc = @import("syscalls/syscall_setup.zig");
const ips = @import("ipc/ipcfn.zig");
const port = @import("ipc/port.zig");
const cid = @import("memory/cpuid_cache.zig");
const cpui = @import("arch/x86_64/cpuid.zig");

extern const KERNEL_VMA: u8;
extern const virt_kernel_start: u8;
extern const virt_kernel_end: u8;

pub const std_options: std.Options = .{
	// Set the log level to debug
	.log_level = .debug,
	.page_size_max = 1*1024*1024*1024,
	// Define logFn to override the std implementation
	.logFn = log.logfn,
};

pub fn hcf() noreturn {
	while(true) {
		asm volatile("hlt");
	}
}
pub fn panic(msg: []const u8, error_return_trace: ?*std.builtin.StackTrace, panic_addr: ?usize) noreturn {
	std.log.err("Error: {s} at {x}\n", .{msg, panic_addr orelse 0});

	if(error_return_trace) |trace| {
		std.log.err("Stack trace:\n", .{});
		for(0..trace.instruction_addresses.len,trace.instruction_addresses) |i, instr| {
			std.log.err("Addr #{d}: 0x{x}\n", .{i, instr});
		}
	}
	hcf();
}

export fn kmain() callconv(.C) void {
	main() catch |err| @panic(@errorName(err));
}

const setup_error = error {
	NO_MULTIPROCESSOR,
	FRAMEBUFFER_NOT_PRESENT,
	MEMORY_MAP_NOT_PRESENT,
	SERIAL_UNAVAILABLE,
	HHDM_NOT_PRESENT
};

var pagealloc: framemanager.PageFrameAllocator = undefined;
var pm: pagemanager.PageManager = undefined;
pub var km: kmm.KernelMemoryManager = undefined;
var acpiman: acpi.ACPIManager = undefined;
var picc: pic.PIC = undefined;
var framebuffer: *limine.Framebuffer = undefined;
var fb_ptr: [*]volatile u32 = undefined;

var lock = spin.SpinLock.init();
var mp_cores: u64 = undefined;
var ifd = std.atomic.Value(u32).init(0);
var barrier1 = std.atomic.Value(bool).init(false);
var barrier2 = std.atomic.Value(bool).init(false);

fn stage_0_sync(arg: ?*anyopaque) void {
	_ = arg;
	const n_cores = ifd.load(.seq_cst);
	if(n_cores == mp_cores - 1) {
		ipi.IPIProtocolHandler.send_ipi_broadcast(ipi.IPIProtocolPayload.init_with_data(.LAPIC_TIMER_SYNC_STAGE_1,0,0,0));
		var lpic = lpicmn.LAPICManager.get_lapic(0);
		ifd.store(0, .seq_cst);
		lpic.set_on_timer(stage_1_sync, null);
		barrier1.store(true, .seq_cst);
	}
}

fn stage_1_sync(arg: ?*anyopaque) void {
	_ = arg;
	const n_cores = ifd.load(.seq_cst);
	if(n_cores == mp_cores - 1) {
		ipi.IPIProtocolHandler.send_ipi_broadcast(ipi.IPIProtocolPayload.init_with_data(.LAPIC_TIMER_SYNC_STAGE_2,0,0,0));
		var lpic = lpicmn.LAPICManager.get_lapic(0);
		lpic.set_on_timer(&schman.SchedulerManager.on_irq, null);
		barrier2.store(true, .seq_cst);
	}
}

fn setup_local_apic_timer(pi: *pic.PIC, hhdm_offset: usize, cpuid: u64, is_bsp: bool) !*lapic.LAPIC {
	const lapic_base = lapic.lapic_physaddr();
	const lapic_virt = lapic_base + hhdm_offset;
	var pitt: pit.PIT = undefined;
	if(is_bsp) {
		try km.pm.map_4k_alloc(km.pm.root_table.?, lapic_base, lapic_virt, km.pfa,.{.pcd = 1});
	}
	try setup_mycpuid();
	var lapicc = lpicmn.LAPICManager.init_lapic(cpuid, lapic_virt, lapic_base, is_bsp);
	if(is_bsp) {
		pi.enable_irq(0);
		pitt = pit.PIT.init();
		pi.set_irq_handler(0, @ptrCast(&pitt), pit.PIT.on_irq);
		idt.enable_interrupts();

		lapicc.init_timer_bsp(1, &pitt);
		lapicc.set_on_timer(stage_0_sync, null);
		pitt.disable();
		pi.disable_irq(0);
		idt.disable_interrupts();
		pi.set_irq_handler(0x10, null, &lpicmn.LAPICManager.on_irq);
		pi.set_irq_handler(0x11, null, &ipi.IPIProtocolHandler.handle_ipi);
	} else {
		_ = ifd.fetchAdd(1, .seq_cst);
		idt.enable_interrupts();
		//asm volatile("hlt");
		while (!barrier1.load(.seq_cst)) {
			asm volatile("pause");
		}
		lapicc.init_timer_ap_stage_1();
		_ = ifd.fetchAdd(1, .seq_cst);
		//asm volatile("hlt");
		while (!barrier2.load(.seq_cst)) {
			asm volatile("pause");
		}
		idt.disable_interrupts();
		lapicc.init_timer_ap_stage_2();
	}
	return lapicc;
}

var supports_rdtscp: std.atomic.Value(bool) = std.atomic.Value(bool).init(false);
var setup_complete: std.atomic.Value(bool) = std.atomic.Value(bool).init(false);
var nsetups: std.atomic.Value(u32) = std.atomic.Value(u32).init(0);

pub fn setup_mycpuid() !void {
	const id = mycpuid_lapic();
	if(id == 0) {
		const has_rdtscp = cpui.cpuid(0x80000001, 0);
		supports_rdtscp.store((has_rdtscp.edx & (1 << 27)) != 0, .release);
	}
	if(supports_rdtscp.load(.acquire)) {
		assm.write_msr(0xC0000103, id);
	} else {
		if(id == 0) {
			try cid.LapicCPUIDCache.init(mp_cores, &km);
		}
		const base = cid.LapicCPUIDCache.get_gs(id);
		assm.write_msr(0xC0000101, base);
	}
	const prev = nsetups.fetchAdd(1, .acq_rel);
	if(prev == mp_cores - 1) {
		setup_complete.store(true, .release);
	}
}

pub fn mycpuid() u64 {
	if(!setup_complete.load(.acquire)) return mycpuid_lapic();
	return if(supports_rdtscp.load(.acquire)) mycpuid_rdtscp() else mycpuid_gs();
}

inline fn mycpuid_rdtscp() u32 {
	return asm volatile (
		\\rdtscp
		\\lfence
		: [aux]"={ecx}"(->u32)
		:
		: "rax", "rdx", "ecx", "memory"
	);
}

pub fn mycpuid_lapic() u64 {
	const lapic_base = lapic.lapic_physaddr();
	const lapic_virt = lapic_base + km.hhdm_offset;
	return lapic.LAPIC.get_cpuid(lapic_virt);
}

inline fn mycpuid_gs() u32 {
	return asm volatile(
		\\movl %%gs:0, %[id]
		\\lfence
		: [id]"=r"(->u32)
		:
		: 
	);
}

var pe = port.Port{};
var pe2 = port.Port{};

fn test1() void {
	const sched = schman.SchedulerManager.get_scheduler_for_cpu(mycpuid());
	pe.owner.store(sched.current_process.?, .release);
	sched.current_process.?.add_port(&pe) catch {};
	sched.current_process.?.add_port(&pe2) catch {};
	for(0..1) |_| {
		const msg = ips.ipc_msg.ipc_message_t{
			.source = 0,
			.dest = 1,
			.flags = 0,
			.npages = 0,
			.page = 0,
			.value = 1
		};
		_ = ips.ipc_send(&msg);
	}
	sched.exit(sched.current_process.?);
}

fn test2() void {
	const sched = schman.SchedulerManager.get_scheduler_for_cpu(mycpuid());
	pe2.owner.store(sched.current_process.?, .release);
	sched.current_process.?.add_port(&pe) catch {};
	sched.current_process.?.add_port(&pe2) catch {};
	const p1 = assm.rdtsc();
	for(0..1) |_| {
		var msg = ips.ipc_msg.ipc_message_t{
			.dest = 1,
			.source = 0
		};
		_ = ips.ipc_recv(&msg);
	}
	const p2 = assm.rdtsc();
	const diff = p2 - p1;
	std.log.info("10 iterations in {} cycles ({} cycles average)", .{diff, diff/10});
	sched.exit(sched.current_process.?);
}

fn on_priority_boost() void {
	var sched = schman.SchedulerManager.get_scheduler_for_cpu(mycpuid());
	while(true) {
		std.log.info("Priority boosting on cpu #{} (Load average: {})", .{mycpuid(), sched.get_load()});
		sched.sleep(1000, sched.current_process.?);
		sched.lock();
		for(1..sch.queue_len) |i| {
			while(sched.queues[i].head != null) {
				const t = sched.queues[i].head;
				sched.remove_task_from_list(t.?, &sched.queues[i]);
				sched.add_task_to_list(t.?, &sched.queues[0]);
			}
		}
		sched.unlock();
		lb.LoadBalancer.steal_task_async();
	}
}

fn idle() void {
	while(true) {
		lb.LoadBalancer.steal_task_async();
		asm volatile("hlt");
	}
}

var colorpoint_id: u64 = 0;
fn colorpoint() void {
	colorpoint_id += 1;
	const mask: u3 = @truncate(colorpoint_id);
	var color: u32 = 0;
	for(0..3) |i| {
		const ir: u2 = @truncate(i);
		const bit = (mask >> (ir)) & 1;
		const nibble: u32 = if (bit == 1) 0xFF else 0x0;
		color |= nibble << (@as(u5, ir) << 3);
	}
	fb_ptr[colorpoint_id << 4] = color;
}

fn cleanup() void {
	while(true) {
		std.log.debug("Deleting dead tasks", .{});
		var sched = schman.SchedulerManager.get_scheduler_for_cpu(mycpuid());
		sched.clear_deleted_tasks();
		sched.block(sched.current_process.?, tsk.TaskStatus.SLEEPING);
	}
}

fn main() !void {
	serial.Serial.init() catch return setup_error.SERIAL_UNAVAILABLE;
	std.log.info("Kernel start: 0x{x}, kernel end: 0x{x} ({} bytes)", .{&virt_kernel_start, &virt_kernel_end,
		@intFromPtr(&virt_kernel_end) - @intFromPtr(&virt_kernel_start)});

	if (requests.framebuffer_request.response) |framebuffer_response| {
		framebuffer = framebuffer_response.getFramebuffers()[0];
		fb_ptr = @ptrCast(@alignCast(framebuffer.address));
	} else {
		return setup_error.FRAMEBUFFER_NOT_PRESENT;
	}

	const offset: usize = if(requests.hhdm_request.response) |response| response.offset else {return setup_error.HHDM_NOT_PRESENT;};
	if(requests.hhdm_request.response) |response|{
		std.log.info("HHDM offset: 0x{x}", .{response.offset});
	} else {
		return setup_error.HHDM_NOT_PRESENT;
	}

	if(requests.memory_map_request.response) |response| {
		pagealloc = try framemanager.PageFrameAllocator.init();
		pm = try pagemanager.PageManager.init(4, offset);
		km = try kmm.KernelMemoryManager.init(
			&pagealloc, 
			&pm,
			offset, 
			@intFromPtr(&virt_kernel_start), 
			@intFromPtr(&virt_kernel_end) - @intFromPtr(&virt_kernel_start)
		);
		try km.setup_frame_allocator(response);
		try km.setup_virtual_stage_one(response);
	} else {
		return setup_error.MEMORY_MAP_NOT_PRESENT;
	}
	if(requests.mp_request.response) |mp_response| {
		mp_cores = mp_response.cpu_count;
		try tss.alloc(mp_cores, &km);
		try gdt.alloc(mp_cores, &km);
		gdt.init(0);
		idt.init();
	}
	if(requests.rsdp_request.response) |rsdp_resp| {
		try pm.map_4k(pm.root_table.?, rsdp_resp.address & ~(@as(u64, 0xFFF)), (rsdp_resp.address + offset) & ~(@as(u64, 0xFFF)), .{});
		acpiman = try acpi.ACPIManager.init(rsdp_resp.revision, rsdp_resp.address + offset, offset, &km);
	}
	idt.set_enable_interrupts();
	picc = pic.PIC.init();
	picc.disable();
	if(requests.mp_request.response != null) {
		try lpicmn.LAPICManager.ginit(mp_cores, &km);
		try schman.SchedulerManager.ginit(mp_cores, &km);
		try ipi.IPIProtocolHandler.ginit(mp_cores, &km);
	}
	_ = try setup_local_apic_timer(&picc, offset, 0, true);

	try ta.TimerAllocator.init();
	try talloc.TaskAllocator.init(1000, mp_cores, &km);
	try sa.KernelStackAllocator.init(1000, mp_cores, &km);
	try fpu.FPUBufferAllocator.init(1000, mp_cores, &km);
	try ioa.IOBufferAllocator.init(6, mp_cores, &km);
	try pta.PageTableAllocator.init(1000, mp_cores, &km);
	try pa.PortAllocator.init(2000, mp_cores, &km);
	try pca.PortChunkAllocator.init(6, mp_cores, &km);
	sysc.setup_syscalls();

	const p = pca.PortChunkAllocator.alloc();
	std.debug.assert(p != null);
	try pca.PortChunkAllocator.free(p.?);

	if(requests.mp_request.response) |mp_response| {
		std.log.info("Available: {} CPUs", .{mp_cores});
		std.log.info("BSP: {}", .{mp_response.bsp_lapic_id});
		for(mp_response.getCpus()) |cpu| {
			std.log.info("Cpu #{}: {}", .{cpu.processor_id, cpu.lapic_id});
			cpu.goto_address = @ptrCast(&ap_begin);
		}
	} else {
		return setup_error.NO_MULTIPROCESSOR;
	}

	var rsp: u64 = undefined;
	asm volatile(
		\\mov %rsp, %[rsp]
		: [rsp] "=r"(rsp)
		:
		:
	);

	var idle_task = tsk.Task.init_idle_task(
		rsp,
		assm.read_cr3(),
	);

	const kernel_stack_priority_boost = sa.KernelStackAllocator.alloc().?;
	var priority_boost = tsk.Task.init_kernel_task(
		on_priority_boost,
		kernel_stack_priority_boost,
		@ptrFromInt(assm.read_cr3()),
		&km
	);
	const cleanup_stack = sa.KernelStackAllocator.alloc().?;
	var cleanup_task = tsk.Task.init_kernel_task(
		cleanup,
		cleanup_stack,
		@ptrFromInt(assm.read_cr3()),
		&km
	);

	nm.setup_supports_avx();
	nm.enable_vector();

	var sched = schman.SchedulerManager.get_scheduler_for_cpu(0);	
	sched.add_idle(&idle_task);
	sched.add_priority_boost(&priority_boost);
	sched.add_cleanup(&cleanup_task);
	const kernel_stack = sa.KernelStackAllocator.alloc().?;
	const test_task = talloc.TaskAllocator.alloc().?;
	test_task.* = tsk.Task.init_kernel_task(
		test1,
		kernel_stack,
		@ptrFromInt(assm.read_cr3()),
		&km
	);
	
	const kernel_stack2 = sa.KernelStackAllocator.alloc().?;
	const test_task2 = talloc.TaskAllocator.alloc().?;
	test_task2.* = tsk.Task.init_kernel_task(
		test2,
		kernel_stack2,
		@ptrFromInt(assm.read_cr3()),
		&km
	);
	tasadd.TaskAdder.add_task(test_task);
	tasadd.TaskAdder.add_task(test_task2);
	idt.enable_interrupts();
	sched.schedule(false);

	idle();
}

const ap_data_struct = packed struct {
	processor_id: u32,
	lapic_id: u32
};

pub fn ap_begin(arg: *requests.SmpInfo) callconv(.C) void {
	ap_start(arg) catch |err| @panic(@errorName(err));
}

pub fn ap_start(arg: *requests.SmpInfo) !void {
	const ap_data: ap_data_struct = .{.processor_id = arg.processor_id, .lapic_id = arg.lapic_id};
	
	gdt.init(ap_data.processor_id);
	idt.init();
	pagemanager.set_cr3(@intFromPtr(km.pm.root_table.?) - km.pm.hhdm_offset);
	var lapicc = try setup_local_apic_timer(&picc, km.pm.hhdm_offset, ap_data.processor_id, false);
	std.log.info("Hello my name is {} (Reported cpuid: {})", .{ap_data.processor_id, mycpuid()});

	var rsp: u64 = undefined;
	asm volatile(
		\\mov %rsp, %[rsp]
		: [rsp] "=r"(rsp)
		:
		:
	);
	var idle_task = tsk.Task.init_idle_task(
		rsp,
		assm.read_cr3(),
	);
	sysc.setup_syscalls();

	nm.setup_supports_avx();
	nm.enable_vector();
	var sched = schman.SchedulerManager.get_scheduler_for_cpu(mycpuid());
	
	const kernel_stack_priority_boost = sa.KernelStackAllocator.alloc().?;
	var priority_boost = tsk.Task.init_kernel_task(
		on_priority_boost,
		kernel_stack_priority_boost,
		@ptrFromInt(assm.read_cr3()),
		&km
	);
	const cleanup_stack = sa.KernelStackAllocator.alloc().?;
	var cleanup_task = tsk.Task.init_kernel_task(
		cleanup,
		cleanup_stack,
		@ptrFromInt(assm.read_cr3()),
		&km
	);
	sched.add_idle(&idle_task);
	
	sched.add_cleanup(&cleanup_task);
	sched.add_priority_boost(&priority_boost);
	lapicc.set_on_timer(@ptrCast(&schman.SchedulerManager.on_irq), null);
	idt.enable_interrupts();
	sched.schedule(false);
	idle();
}
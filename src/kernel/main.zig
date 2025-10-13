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
	var writer = serial.Serial.writer();
	writer.print("\nError: {s} at {x}\n", .{msg, panic_addr orelse 0}) catch {
		serial.Serial.write('a');
		hcf();
	};
	if(error_return_trace) |trace| {
		_ = writer.print("Stack trace:\n", .{}) catch {
				serial.Serial.write('b');
				hcf();
			};
		for(0..trace.instruction_addresses.len,trace.instruction_addresses) |i, instr| {
			_ = writer.print("Addr #{d}: 0x{x}\n", .{i, instr}) catch {
				serial.Serial.write('c');
				hcf();
			};
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
var km: kmm.KernelMemoryManager = undefined;
var acpiman: acpi.ACPIManager = undefined;
var picc: pic.PIC = undefined;
var framebuffer: *limine.Framebuffer = undefined;
var fb_ptr: [*]volatile u32 = undefined;


var lock = spin.SpinLock.init();
var mp_cores: u64 = undefined;
var ifd = std.atomic.Value(bool).init(false);
var has_pic: u64 = 0;
var np: u64 = 0;

fn setup_local_apic_timer(pi: *pic.PIC, hhdm_offset: usize, cpuid: u64, is_bsp: bool) !*lapic.LAPIC {
	const lapic_base = lapic.lapic_physaddr();
	const lapic_virt = lapic_base + hhdm_offset;
	var pitt: pit.PIT = undefined;
	if(is_bsp) {
		try km.pm.map_4k_alloc(km.pm.root_table.?, lapic_base, lapic_virt, km.pfa,.{.pcd = 1});
	}
	
	var lapicc = lpicmn.LAPICManager.init_lapic(cpuid, lapic_virt, lapic_base, is_bsp);
	if(is_bsp) {
		pi.enable_irq(0);
		pitt = pit.PIT.init();
		pi.set_irq_handler(0, @ptrCast(&pitt), pit.PIT.on_irq);
		idt.enable_interrupts();

		lapicc.init_timer_bsp(1, &pitt);
		lapicc.arg = null;
		lapicc.timer_event.store(null, .release);
		pitt.disable();
		pi.disable_irq(0);
		idt.disable_interrupts();
		pi.set_irq_handler(0x10, null, &lpicmn.LAPICManager.on_irq_bsp);
	} else {
		// IPI handler
		// I'd love to set up local timer but it has proven to be too complicated (for some reason atomics on interrupt work like shit)
		// Maybe I will tackle this some time in the future
		pi.set_irq_handler(0x11, null, &lpicmn.LAPICManager.on_irq);
	}
	return lapicc;
}

pub fn mycpuid() u64 {
	const lapic_base = lapic.lapic_physaddr();
	const lapic_virt = lapic_base + km.hhdm_offset;
	return lapic.LAPIC.get_cpuid(lapic_virt);
}

pub fn mask_ints() void {
	const id = mycpuid();
	const lapicc = lpicmn.LAPICManager.get_lapic(id);
	if(id == 0) {
		lapicc.mask_irq(0);
	} else {
		lapicc.mask_irq(3);
	}
}

pub fn unmask_ints() void {
	const id = mycpuid();
	const lapicc = lpicmn.LAPICManager.get_lapic(id);
	if(id == 0) {
		lapicc.unmask_irq(0);
	} else {
		lapicc.unmask_irq(3);
	}
}

fn test1() void {
	var sched = schman.SchedulerManager.get_scheduler_for_cpu(mycpuid());
	while(true) {
		std.log.info("hey hey! (CPU #{}) (priority {})", .{mycpuid(), sched.get_priority(sched.current_process.?)});
	}
}

fn test2() void {
	var sched = schman.SchedulerManager.get_scheduler_for_cpu(mycpuid());
	var p = true;
	for(0..20000) |_| {
		std.log.info("tururu! (CPU #{}) (priority {})", .{mycpuid(), sched.get_priority(sched.current_process.?)});
		if(sched.blocked_tasks != null) { 
			std.log.info("Yes task :)", .{});
			sched.unblock(sched.blocked_tasks.?);
			std.log.info("Unblocked task", .{});
		} else {
			std.log.info("no task :(", .{});
			if(p) {
				colorpoint();
				p = false;
			}
		}
	}
	colorpoint();
	if(sched.current_process != null) {
		std.log.info("Priority: {}", .{sched.get_priority(sched.current_process.?)});
		colorpoint();
		if(sched.blocked_tasks != null) { 
			sched.exit(sched.blocked_tasks.?);
		} else {
			sched.exit(sched.current_process.?.next.?);
		}
		sched.exit(sched.current_process.?);
	}
}

fn on_priority_boost() void {
	var sched = schman.SchedulerManager.get_scheduler_for_cpu(mycpuid());
	while(true) {
		std.log.info("Priority boosting on cpu #{} (Load average: {})", .{mycpuid(), sched.get_load()});
		sched.sleep(1000, sched.current_process.?);
		sched.lock();
		for(1..sch.queue_len) |i| {
			while(sched.queues[i] != null) {
				const t = sched.queues[i];
				sched.remove_task_from_list(t.?, &sched.queues[i]);
				sched.add_task_to_list(t.?, &sched.queues[0]);
			}
		}
		sched.unlock();
	}
}

fn idle() void {
	const sched = schman.SchedulerManager.get_scheduler_for_cpu(mycpuid());
	while(true) {
		lb.LoadBalancer.steal_task(sched);
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
	}
	var lapicc = try setup_local_apic_timer(&picc, offset, 0, true);

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

	try ta.TimerAllocator.init();

	 const kernel_stack_1 = (try km.alloc_virt(tsk.KERNEL_STACK_SIZE/pagemanager.PAGE_SIZE)).?;
	 var test_task_1 = tsk.Task.init_kernel_task(
	 test1,
	 @ptrFromInt(kernel_stack_1 + tsk.KERNEL_STACK_SIZE),
	 @ptrFromInt(kernel_stack_1 + tsk.KERNEL_STACK_SIZE),
	 @ptrFromInt(assm.read_cr3()),
	 &km
	 );

	// const kernel_stack_2 = (try km.alloc_virt(tsk.KERNEL_STACK_SIZE/pagemanager.PAGE_SIZE)).?;
	// var test_task_2 = tsk.Task.init_kernel_task(
	// test2,
	// @ptrFromInt(kernel_stack_2 + tsk.KERNEL_STACK_SIZE),
	// @ptrFromInt(kernel_stack_2 + tsk.KERNEL_STACK_SIZE),
	// @ptrFromInt(assm.read_cr3()),
	// &km
	// );

	const kernel_stack_3 = (try km.alloc_virt(tsk.KERNEL_STACK_SIZE/pagemanager.PAGE_SIZE)).?;
	var priority_boost = tsk.Task.init_kernel_task(
		on_priority_boost,
		@ptrFromInt(kernel_stack_3 + tsk.KERNEL_STACK_SIZE),
		@ptrFromInt(kernel_stack_3 + tsk.KERNEL_STACK_SIZE),
		@ptrFromInt(assm.read_cr3()),
		&km
	);
	const cleanup_stack = (try km.alloc_virt(tsk.KERNEL_STACK_SIZE/pagemanager.PAGE_SIZE)).?;
	var cleanup_task = tsk.Task.init_kernel_task(
		cleanup,
		@ptrFromInt(cleanup_stack + tsk.KERNEL_STACK_SIZE),
		@ptrFromInt(cleanup_stack + tsk.KERNEL_STACK_SIZE),
		@ptrFromInt(assm.read_cr3()),
		&km
	);

	std.log.info("task: {any}", .{idle_task});
	//std.log.info("task: {any}", .{test_task_1});
	//std.log.info("task: {any}", .{test_task_2});
	std.log.info("task: {any}", .{priority_boost});

	var sched = schman.SchedulerManager.get_scheduler_for_cpu(0);	
	sched.add_idle(&idle_task);
	
	sched.add_cleanup(&cleanup_task);
	sched.add_task(&priority_boost);
	test_task_1.is_pinned = false;
	sched.add_task(&test_task_1);
	//sched.add_task(&test_task_2);
	lapicc.set_on_timer(@ptrCast(&schman.SchedulerManager.on_irq), null);
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
	var sched = schman.SchedulerManager.get_scheduler_for_cpu(mycpuid());
	
	const kernel_stack_1 = (try km.alloc_virt(4)).?;
	var priority_boost = tsk.Task.init_kernel_task(
		on_priority_boost,
		@ptrFromInt(kernel_stack_1 + 4*pagemanager.PAGE_SIZE),
		@ptrFromInt(kernel_stack_1 + 4*pagemanager.PAGE_SIZE),
		@ptrFromInt(assm.read_cr3()),
		&km
	);
	const cleanup_stack = (try km.alloc_virt(tsk.KERNEL_STACK_SIZE/pagemanager.PAGE_SIZE)).?;
	var cleanup_task = tsk.Task.init_kernel_task(
		cleanup,
		@ptrFromInt(cleanup_stack + tsk.KERNEL_STACK_SIZE),
		@ptrFromInt(cleanup_stack + tsk.KERNEL_STACK_SIZE),
		@ptrFromInt(assm.read_cr3()),
		&km
	);
	sched.add_idle(&idle_task);
	lapicc.set_on_timer(@ptrCast(&schman.SchedulerManager.on_irq), null);
	sched.add_cleanup(&cleanup_task);
	sched.add_task(&priority_boost);
	idt.enable_interrupts();
	sched.schedule(false);
	idle();
}
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

pub const MAX_CORES = 64;

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

fn setup_local_apic_timer(pi: *pic.PIC, hhdm_offset: usize, cpuid: u64, is_bsp: bool) !*lapic.LAPIC {
	const lapic_base = lapic.lapic_physaddr();
	const lapic_virt = lapic_base + hhdm_offset;
	var pitt: pit.PIT = undefined;
	if(is_bsp) {
		try km.pm.map_4k_alloc(km.pm.root_table.?, lapic_base, lapic_virt, km.pfa);
		pi.enable_irq(0);
		pitt = pit.PIT.init();
		pi.set_irq_handler(0, @ptrCast(&pitt), pit.PIT.on_irq);
		idt.enable_interrupts();
	}
	
	var lapicc = lpicmn.LAPICManager.init_lapic(cpuid, lapic_virt, lapic_base, is_bsp);
	if(is_bsp) { 
		lapicc.init_timer_bsp(10, &pitt);
		pitt.disable();
		idt.disable_interrupts();
		picc.set_irq_handler(0, null, &lpicmn.LAPICManager.on_irq);
	}
	return lapicc;
}

pub fn mycpuid() u64 {
	const lapic_base = lapic.lapic_physaddr();
	const lapic_virt = lapic_base + km.hhdm_offset;
	return lapic.LAPIC.get_cpuid(lapic_virt);
}

fn test1() void {
	while(true) {
		std.log.info("hey hey! (CPU #{})", .{mycpuid()});
		var sched = schman.SchedulerManager.get_scheduler_for_cpu(mycpuid());
		if(sched.current_process) |_| {
			sched.block(sched.current_process.?, tsk.TaskStatus.FINISHED);
		}
	}
}

fn test2() void {
	var v: u64 = 1;
	while(true) {
		std.log.info("tururu (CPU #{})", .{mycpuid()});
		v += 1;
		if(v > 5) {
			var sched = schman.SchedulerManager.get_scheduler_for_cpu(mycpuid());
			if(sched.current_process) |_| {
				sched.block(sched.current_process.?, tsk.TaskStatus.FINISHED);
			}
		}
	}
}

fn main() !void {
	serial.Serial.init() catch return setup_error.SERIAL_UNAVAILABLE;
	std.log.info("Kernel start: 0x{x}, kernel end: 0x{x} ({} bytes)", .{&virt_kernel_start, &virt_kernel_end,
	@intFromPtr(&virt_kernel_end) - @intFromPtr(&virt_kernel_start)});
	var mp_cores: u64 = undefined;

	gdt.init(0);
	idt.init();

	if (requests.framebuffer_request.response) |framebuffer_response| {
		const framebuffer = framebuffer_response.getFramebuffers()[0];
		for (0..256) |i| {
			const fb_ptr: [*]volatile u32 = @ptrCast(@alignCast(framebuffer.address));
			fb_ptr[i * (framebuffer.pitch / 4) + i] = (0xffff << 8) | (@as(u32, @truncate(i & 0xff)));
		}
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
	if(requests.rsdp_request.response) |rsdp_resp| {
		try pm.map_4k(pm.root_table.?, rsdp_resp.address & ~(@as(u64, 0xFFF)), (rsdp_resp.address + offset) & ~(@as(u64, 0xFFF)));
		acpiman = try acpi.ACPIManager.init(rsdp_resp.revision, rsdp_resp.address + offset, offset, &km);
	}
	idt.set_enable_interrupts();
	picc = pic.PIC.init();
	picc.disable();
	if(requests.mp_request.response) |mp_response| {
		mp_cores = mp_response.cpu_count;
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

	const kernel_stack_1 = (try km.alloc_virt(tsk.KERNEL_STACK_SIZE/pagemanager.PAGE_SIZE)).?;
	var test_task_1 = tsk.Task.init_kernel_task(
		test1,
		@ptrFromInt(kernel_stack_1 + tsk.KERNEL_STACK_SIZE),
		@ptrFromInt(kernel_stack_1 + tsk.KERNEL_STACK_SIZE),
		@ptrFromInt(assm.read_cr3()),
		&km
	);

	const kernel_stack_2 = (try km.alloc_virt(tsk.KERNEL_STACK_SIZE/pagemanager.PAGE_SIZE)).?;
	var test_task_2 = tsk.Task.init_kernel_task(
		test2,
		@ptrFromInt(kernel_stack_2 + tsk.KERNEL_STACK_SIZE),
		@ptrFromInt(kernel_stack_2 + tsk.KERNEL_STACK_SIZE),
		@ptrFromInt(assm.read_cr3()),
		&km
	);

	std.log.info("task: {any}", .{idle_task});
	std.log.info("task: {any}", .{test_task_1});
	std.log.info("task: {any}", .{test_task_2});

	var sched = schman.SchedulerManager.get_scheduler_for_cpu(0);
	
	sched.add_idle(&idle_task);
	lapicc.set_on_timer(@ptrCast(&schman.SchedulerManager.on_irq), null);
	
	sched.add_task(&test_task_1);
	sched.add_task(&test_task_2);
	sched.schedule();

	hcf();
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
	var ttask1 = tsk.Task.init_kernel_task(
		test1,
		@ptrFromInt(kernel_stack_1 + 4*pagemanager.PAGE_SIZE),
		@ptrFromInt(kernel_stack_1 + 4*pagemanager.PAGE_SIZE),
		@ptrFromInt(assm.read_cr3()),
		&km
	);
	sched.add_idle(&idle_task);
	sched.add_task(&ttask1);
	lapicc.set_on_timer(@ptrCast(&schman.SchedulerManager.on_irq), null);
	//idt.enable_interrupts();
	//sched.schedule();
	hcf();
}
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

extern const KERNEL_VMA: u8;
extern const virt_kernel_start: u8;
extern const virt_kernel_end: u8;

pub const std_options: std.Options = .{
    // Set the log level to debug
    .log_level = .debug,

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
	writer.print("Error: {s} at {x}\n", .{msg, panic_addr orelse 0}) catch {
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

fn setup_local_apic_timer(pi: *pic.PIC, hhdm_offset: usize, is_bsp: bool) !lapic.LAPIC {
	const lapic_base = lapic.lapic_physaddr();
	const lapic_virt = lapic_base + hhdm_offset;
	if(is_bsp) {
		try km.pm.map_4k_alloc(km.pm.root_table.?, lapic_base, lapic_virt, km.pfa);
		pi.enable_irq(0);
	}
	var sin = spin.SpinLock.init();
	sin.lock();
	defer sin.unlock();
	var pitt = pit.PIT.init();
	pi.set_irq_handler(0, @ptrCast(&pitt), pit.PIT.on_irq);
	idt.enable_interrupts();
	var lapicc = lapic.LAPIC.init(lapic_virt, lapic_base, is_bsp);
	lapicc.init_timer(100, &pitt);
	pitt.disable();
	pi.set_irq_handler(0, @ptrCast(&lapicc), lapic.LAPIC.on_irq);
	return lapicc;
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

	picc = pic.PIC.init();
	picc.disable();
	_ = try setup_local_apic_timer(&picc, offset, true);

	if(requests.mp_request.response) |mp_response| {
		mp_cores = mp_response.cpu_count;
		std.log.info("Available: {} CPUs", .{mp_cores});
		std.log.info("BSP: {}", .{mp_response.bsp_lapic_id});
		for(mp_response.getCpus()) |cpu| {
			std.log.info("Cpu #{}: {}", .{cpu.processor_id, cpu.lapic_id});
			cpu.goto_address = @ptrCast(&ap_begin);
			cpu.extra_argument = offset;
		}
	} else {
		return setup_error.NO_MULTIPROCESSOR;
	}
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
	const hhdm_offset = arg.extra_argument;
	std.log.info("Hello my name is {}", .{ap_data.processor_id});
	gdt.init(ap_data.processor_id);
	idt.init();
	pagemanager.set_cr3(@intFromPtr(km.pm.root_table.?) - km.pm.hhdm_offset);
	_ = try setup_local_apic_timer(&picc, hhdm_offset, false);

	hcf();
}
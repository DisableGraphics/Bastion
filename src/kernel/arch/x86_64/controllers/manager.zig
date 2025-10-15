const lapic = @import("lapic.zig");
const frame = @import("../../../memory/kmm.zig");
const page = @import("../../../memory/pagemanager.zig");
const main = @import("../../../main.zig");
const ipi = @import("../../../interrupts/ipi_protocol.zig");

pub const LAPICManager = struct {
	var lapics: []lapic.LAPIC = undefined;
	pub fn ginit(cpus: u64, allocator: *frame.KernelMemoryManager) !void {
		const pages = ((cpus * @sizeOf(lapic.LAPIC)) + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
		lapics = @as([*]lapic.LAPIC, @ptrFromInt((try allocator.alloc_virt(pages)).?))[0..cpus];
	}

	pub fn init_lapic(cpuid: u64, lapic_table_virtaddr: usize, lapic_base_physaddr: usize, is_bsp: bool) *lapic.LAPIC {
		lapics[cpuid] = lapic.LAPIC.init(lapic_table_virtaddr, lapic_base_physaddr, is_bsp);
		return &lapics[cpuid];
	}

	pub fn get_lapic(cpuid: u64) *lapic.LAPIC {
		return &lapics[cpuid];
	}

	pub fn on_irq(s: ?*volatile anyopaque) void {
		_ = s;
		const cpuid = main.mycpuid();
		const lapicc = &lapics.ptr[cpuid];
		lapic.LAPIC.on_irq(@ptrCast(lapicc));
	}

	pub fn on_irq_bsp(s: ?*volatile anyopaque) void {
		_ = s;
		const cpuid = main.mycpuid();
		const lapicc = &lapics.ptr[cpuid];
		lapic.LAPIC.on_irq(@ptrCast(lapicc));
		for(0..lapics.len) |i| {
			ipi.IPIProtocolHandler.send_ipi(@truncate(i), ipi.IPIProtocolPayload.init_with_data(.SCHEDULE, 0,0,0));
		}
	}
};
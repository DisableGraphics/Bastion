const cpui = @import("../cpuid.zig");
const assm = @import("../asm.zig");
const ppt = @import("../../../memory/per_process_table.zig");
const main = @import("../../../main.zig");
const std = @import("std");
const lapic = @import("../controllers/lapic.zig");
pub const cpuid_t = u64;

var setup_complete: std.atomic.Value(bool) = std.atomic.Value(bool).init(false);
var supports_rdtscp: std.atomic.Value(bool) = std.atomic.Value(bool).init(false);
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
		ppt.PerProcessTable.get_my_table().mycpuid = mycpuid();
	}
	const prev = nsetups.fetchAdd(1, .acq_rel);
	if(prev == main.mp_cores - 1) {
		setup_complete.store(true, .release);
	}
}

pub fn mycpuid() cpuid_t {
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

fn mycpuid_lapic() cpuid_t {
	const lapic_base = lapic.lapic_physaddr();
	const lapic_virt = lapic_base + main.km.hhdm_offset;
	return @truncate(lapic.LAPIC.get_cpuid(lapic_virt));
}

inline fn mycpuid_gs() u32 {
	return @truncate(ppt.PerProcessTable.get_my_table().mycpuid);
}

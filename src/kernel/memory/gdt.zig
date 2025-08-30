const tss = @import("tss.zig");
const main = @import("../main.zig");

fn encodeGdtEntry(limit: u32, base: u32, access_byte: u8, flags: u8) u64 {
	return (@as(u64, limit & 0xFFFF))
		| (@as(u64, base & 0xFFFFFF) << 16)
		| (@as(u64, access_byte) << 40)
		| (@as(u64, ((limit >> 16) & 0x0F) | ((flags & 0x0F) << 4)) << 48)
		| (@as(u64, (base >> 24) & 0xFF) << 56);
}

const gdt_type = [6]u64;

const gdtr = packed struct {
	limit: u16,
	base: u64,
};

var gdts: [main.MAX_CORES]gdt_type = undefined;
var gdtr_reg: [main.MAX_CORES]gdtr = undefined;

fn set_gdtr(addr: *const gdt_type, gdtreg: *gdtr) void {
	const size = @sizeOf(gdt_type);
	gdtreg.limit = size - 1;
	gdtreg.base = @intFromPtr(addr);
	load_gdt(gdtreg);
}

fn load_gdt(gdtreg: *const gdtr) void {
	asm volatile ("lgdt (%[gdtreg])"
		:
		: [gdtreg] "r" (gdtreg)
		: "memory"
	);
}

extern fn reloadSegments() callconv(.C) void;

pub fn gdt_init(local_gdt: *gdt_type, gdtreg: *gdtr, core_id: u32) void {
	local_gdt[0] = encodeGdtEntry(0, 0, 0, 0); // Null entry
	local_gdt[1] = encodeGdtEntry(0xFFFFF, 0, 0x9A, 0xA);
	local_gdt[2] = encodeGdtEntry(0xFFFFF, 0, 0x92, 0xC);
	local_gdt[3] = encodeGdtEntry(0xFFFFF, 0, 0xFA, 0xA);
	local_gdt[4] = encodeGdtEntry(0xFFFFF, 0, 0xF2, 0xC);
	local_gdt[5] = encodeGdtEntry(@sizeOf(tss.tss_t) - 1, @truncate(@intFromPtr(tss.get_tss(core_id))), 0x89, 0);
	set_gdtr(local_gdt, gdtreg);
}

pub fn init(core_id: u32) void {
	tss.init(core_id);
	gdt_init(&gdts[core_id], &gdtr_reg[core_id], core_id);
	reloadSegments();
}

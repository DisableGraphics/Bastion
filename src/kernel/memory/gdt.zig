const tss = @import("tss.zig");
const main = @import("../main.zig");
const kmm = @import("kmm.zig");
const page = @import("pagemanager.zig");

fn encodeGdtEntry(limit: u32, base: u32, access_byte: u8, flags: u8) u64 {
	return (@as(u64, limit & 0xFFFF))
		| (@as(u64, base & 0xFFFFFF) << 16)
		| (@as(u64, access_byte) << 40)
		| (@as(u64, ((limit >> 16) & 0x0F) | ((flags & 0x0F) << 4)) << 48)
		| (@as(u64, (base >> 24) & 0xFF) << 56);
}

const gdt_type = [7]u64;

const gdtr = packed struct {
	limit: u16,
	base: u64,
};

pub var gdts: []gdt_type = undefined;

fn load_gdt(gdtreg: *const gdtr) void {
	asm volatile ("lgdt (%[gdtreg])"
		:
		: [gdtreg] "r" (gdtreg)
		: "memory"
	);
}

extern fn reloadSegments() callconv(.C) void;

pub fn gdt_init(local_gdt: *gdt_type, core_id: u32) void {
	const size = @sizeOf(gdt_type);
	const gdtreg: gdtr = .{.limit = size - 1, .base = @intFromPtr(local_gdt)};
	local_gdt[0] = encodeGdtEntry(0, 0, 0, 0); // Null entry
	local_gdt[1] = encodeGdtEntry(0xFFFFF, 0, 0x9A, 0xA);
	local_gdt[2] = encodeGdtEntry(0xFFFFF, 0, 0x92, 0xC);
	local_gdt[3] = encodeGdtEntry(0xFFFFF, 0, 0xF2, 0xC);
	local_gdt[4] = encodeGdtEntry(0xFFFFF, 0, 0xFA, 0xA);
	const low32bit: u32 =  @truncate(@intFromPtr(tss.get_tss(core_id)));
	local_gdt[5] = encodeGdtEntry(@sizeOf(tss.tss_t) - 1, low32bit, 0x89, 0);
	const high32bit: u32 =  @truncate(@intFromPtr(tss.get_tss(core_id)) >> 32);
	local_gdt[6] = high32bit;
	load_gdt(&gdtreg);
}

pub fn alloc(ncores: u64, allocator: *kmm.KernelMemoryManager) !void {
	const pages_gdt = ((ncores * @sizeOf(gdt_type)) + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
	const ptr = (try allocator.alloc_virt(pages_gdt)).?;
	gdts = @as([*]gdt_type, @ptrFromInt(ptr))[0..ncores];
}

pub fn init(core_id: u32) void {
	tss.init(core_id);
	gdt_init(&gdts[core_id], core_id);
	reloadSegments();
}

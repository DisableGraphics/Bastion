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

fn encodeTssDescriptor(base: u64, limit: u32) [2]u64 {
	var low: u64 = 0;
	var high: u64 = 0;

	low |= (limit & 0xFFFF);
	low |= (base & 0xFFFFFF) << 16;
	low |= (@as(u64, 0x89)) << 40;
	low |= @as(u64, ((limit >> 16) & 0xF)) << 48;
	
	low |= ((base >> 24) & 0xFF) << 56;

	high = base >> 32;

	return .{ low, high };
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
	const base = @intFromPtr(tss.get_tss(core_id));
	const desc = encodeTssDescriptor(base, @sizeOf(tss.tss_t));

	local_gdt[5] = desc[0];
	local_gdt[6] = desc[1];
	load_gdt(&gdtreg);
	tss.load_tss();
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

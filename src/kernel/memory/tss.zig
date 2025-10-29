const main = @import("../main.zig");
const kmm = @import("kmm.zig");
const page = @import("pagemanager.zig");
const std = @import("std");

const IO_BITMAP_SIZE = 8192;
pub const io_bitmap_t = [IO_BITMAP_SIZE]u8;
pub const tss_t = extern struct { 
	reserved_1: u32, 
	rsp0: u64 align(4), 
	rsp1: u64 align(4), 
	rsp2: u64 align(4), 
	reserved_2: u64 align(4), 
	ist1: u64 align(4), 
	ist2: u64 align(4), 
	ist3: u64 align(4), 
	ist4: u64 align(4), 
	ist5: u64 align(4), 
	ist6: u64 align(4), 
	ist7: u64 align(4), 
	reserved_3: u64 align(4), 
	reserved_4: u16, 
	iopb: u16,
	io_bitmap: io_bitmap_t,
	end_of_bitmap: u8
};
var tss_s: []tss_t = undefined;
pub fn init(core_id: u32) void {
	tss_s[core_id] = .{
		.reserved_1 = 0,
		.rsp0 = 0,
		.rsp1 = 0,
		.rsp2 = 0,
		.reserved_2 = 0,
		.ist1 = 0,
		.ist2 = 0,
		.ist3 = 0,
		.ist4 = 0,
		.ist5 = 0,
		.ist6 = 0,
		.ist7 = 0,
		.reserved_3 = 0,
		.reserved_4 = 0,
		.iopb = 65535,
		.io_bitmap = [_]u8{0xFF} ** (IO_BITMAP_SIZE),
		.end_of_bitmap = 0xff
	};
}

pub fn alloc(ncores: u64, allocator: *kmm.KernelMemoryManager) !void {
	const pages_tss = ((ncores * @sizeOf(tss_t)) + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
	const ptr = (try allocator.alloc_virt(pages_tss)).?;
	tss_s = @as([*]tss_t, @ptrFromInt(ptr))[0..ncores];
}

pub fn get_tss(core_id: u32) *tss_t {
    return &tss_s[core_id];
}

const main = @import("../main.zig");

pub const tss_t = packed struct { 
	reserved_1: u32, 
	rsp0: u64, 
	rsp1: u64, 
	rsp2: u64, 
	reserved_2: u64, 
	ist1: u64, 
	ist2: u64, 
	ist3: u64, 
	ist4: u64, 
	ist5: u64, 
	ist6: u64,
	ist7: u64, 
	reserved_3: u64, 
	reserved_4: u16, 
	iopb: u16 
};
var tss_s: [main.MAX_CORES]tss_t = undefined;
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
		.iopb = 0
	};
}

pub fn get_tss(core_id: u32) *tss_t {
    return &tss_s[core_id];
}

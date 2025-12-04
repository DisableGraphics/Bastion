const kmm = @import("kmm.zig");
const page = @import("pagemanager.zig");
const assm = @import("../arch/x86_64/asm.zig");

pub const PerProcessData = extern struct {
	self_ptr: u64,
	kernel_stack: u64,
	mycpuid: u64,
	oldrcx: u64,
};

pub const PerProcessTable = struct {
	var table: []PerProcessData = undefined;
	pub fn ginit(mp_cores: u64, km: *kmm.KernelMemoryManager) !void {
		const table_pages = ((mp_cores * @sizeOf(PerProcessData)) + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
		table = @as([*]PerProcessData, @ptrFromInt((try km.alloc_virt(table_pages)).?))[0..mp_cores];
	}

	pub fn setup_on_cpu(mycpuid: u64) void {
		const ptr = @intFromPtr(&table[mycpuid]);
		table[mycpuid].self_ptr = ptr;
		assm.write_msr(0xC0000102, ptr);
	}

	pub fn get_my_table() *PerProcessData {
		const ptr: *PerProcessData = asm volatile(
			\\swapgs
			\\movq %gs:0, %[dest]
			\\swapgs
			: [dest]"=r"(-> *PerProcessData)
			:
			:
		);
		return ptr;
	}
};
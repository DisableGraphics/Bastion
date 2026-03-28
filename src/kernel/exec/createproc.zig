const tsk = @import("../scheduler/task.zig");
const tska = @import("../scheduler/taskalloc.zig");
const ld = @import("loadmod.zig");
const ksa = @import("../memory/stackalloc.zig");
const kmm = @import("../memory/kmm.zig");
const page = @import("../memory/pagemanager.zig");

pub fn create_mmanager_process(data: ld.MManagerData, km: *kmm.KernelMemoryManager) !*tsk.Task {
	const task = tska.TaskAllocator.alloc() orelse return error.NO_SPACE_LEFT;
	const ks = ksa.KernelStackAllocator.alloc() orelse return error.NO_STACK_SPACE_LEFT;
	task.* = try tsk.Task.init_user_task(ks, data.addrsp);
	// Allocate user space stack 
	const N_PAGES_USER_STACK = 64;
	const virtaddr = 0x0000_8000_0000_0000; // Stack pointer (i know its non-canonical but when it pushes things on stack it will be canonical)
	const physaddr = (try km.alloc_phys(N_PAGES_USER_STACK)) orelse return error.NO_SPACE_LEFT;

	for(0..N_PAGES_USER_STACK) |i| {
		const page_physaddr_addr = physaddr + (i * page.PAGE_SIZE);
		const page_virtual_addr = virtaddr - ((i+1)*page.PAGE_SIZE);
		try data.addrsp.add_mapping_4k(page_physaddr_addr, page_virtual_addr, 
		.{
			.xd = 1,
			.us = 1,
			.rw = 1,
		}, .{
			.xd = 1,
			.us = 1,
			.rw = 1,
		}, .{
			.xd = 1,
			.us = 1,
			.rw = 1,
		}, .{
			.xd = 1,
			.us = 1,
			.rw = 1,
		});
	}

	const sp_addr: *anyopaque = @ptrFromInt(virtaddr);

	try task.start_user_task(@ptrFromInt(data.start_vaddr), sp_addr, tsk.tls_registers.init(0, 0));
	return task;
}

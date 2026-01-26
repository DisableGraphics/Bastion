const assm = @import("../arch/x86_64/asm.zig");
const std = @import("std");
const gdt = @import("../memory/gdt.zig");
const ipcfn = @import("../ipc/ipcfn.zig");
const main = @import("../main.zig");
const task = @import("../scheduler/task.zig");
const schman = @import("../scheduler/manager.zig");
const portalloc = @import("../ipc/portalloc.zig");
const taskalloc = @import("../scheduler/taskalloc.zig");
const taskadder = @import("../scheduler/task_adder.zig");
const kstackalloc = @import("../memory/stackalloc.zig");
const page = @import("../memory/pagemanager.zig");
const port = @import("../ipc/port.zig");
const ipi = @import("../interrupts/ipi_protocol.zig");
const inttable = @import("../interrupts/iporttable.zig");
const iob = @import("../memory/io_bufferalloc.zig");
const asa = @import("../memory/addrspacealloc.zig");
const as = @import("../memory/addrspace.zig");
const pp = @import("../memory/physicalpage.zig");

const IA32_EFER = 0xC0000080;
const IA32_STAR = 0xC0000081;
const IA32_LSTAR = 0xC0000082;
const IA32_SFMASK = 0xC0000084;

extern fn syscall_handler() callconv(.C) void;

pub fn setup_syscalls() void {
	const star: u64 = (0x13 << 48) | (0x8 << 32);
	
	assm.write_msr(IA32_SFMASK, 0x200); // Disable interrupts while syscall is handled
	assm.write_msr(IA32_LSTAR, @intFromPtr(&syscall_handler));
	assm.write_msr(IA32_STAR, star); // Save code segments
	assm.write_msr(IA32_EFER, assm.read_msr(IA32_EFER) | 1); // enable syscall
}

const ret_t = isize;

fn create_port(ptr: *ipcfn.ipc_msg.ipc_message_t) ret_t {
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
	const prt = portalloc.PortAllocator.alloc();
	if(prt) |p| {
		p.* = port.Port.init();
		const prtno = sch.current_process.?.add_port(p) catch ipcfn.ipc_msg.ENODEST;
		ptr.value0 = @intCast(prtno);
		return ipcfn.ipc_msg.EOK;
	} else {
		return ipcfn.ipc_msg.ENODEST;
	}
}

fn create_process(ptr: *ipcfn.ipc_msg.ipc_message_t) ret_t {
	const meself = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid()).current_process.?;
	// For all the things I need (task, bootstrapping port & kernel stack), try to allocate them
	// and return if there are no options available.
	const newtask = taskalloc.TaskAllocator.alloc();
	if(newtask != null) {
		const bootstrapprt = portalloc.PortAllocator.alloc();
		if(bootstrapprt) |p| {
			const kernel_stack = kstackalloc.KernelStackAllocator.alloc();
			if(kernel_stack) |k| {
				p.* = port.Port.init();
				_ = meself.addr_space.?.refcount.fetchAdd(1, .acq_rel);
				// Try to initialize the task, and if it fails, free everything.
				newtask.?.* = task.Task.init_user_task(
					k,
					@ptrFromInt(page.get_cr3()),
					meself.addr_space.?
				) catch {
					portalloc.PortAllocator.free(p) catch {};
					taskalloc.TaskAllocator.free(newtask.?) catch {};
					kstackalloc.KernelStackAllocator.free(k) catch {};
					return ipcfn.ipc_msg.ENODEST;
				};
				newtask.?.state = task.TaskStatus.STARTING;

				// Add the bootstrap port to the new task. If it fails, free everything
				const newtaskport = newtask.?.add_port(p) catch {
					portalloc.PortAllocator.free(p) catch {};
					taskalloc.TaskAllocator.free(newtask.?) catch {};
					kstackalloc.KernelStackAllocator.free(k) catch {};
					return ipcfn.ipc_msg.ENODEST;
				};
				p.owner.store(newtask.?, .seq_cst);
				// Make sure that this new task port is 0
				std.debug.assert(newtaskport == 0);

				// Finally add the new task port to the ports in the parent
				const portindest = meself.add_port(p) catch {
					portalloc.PortAllocator.free(p) catch {};
					taskalloc.TaskAllocator.free(newtask.?) catch {};
					kstackalloc.KernelStackAllocator.free(k) catch {};
					return ipcfn.ipc_msg.ENODEST;
				};

				ptr.value0 = @intCast(portindest);

				taskadder.TaskAdder.add_blocked_task(newtask.?);

				return ipcfn.ipc_msg.EOK;
			} else {
				portalloc.PortAllocator.free(p) catch {};
				taskalloc.TaskAllocator.free(newtask.?) catch {};
			}
		} else {
			taskalloc.TaskAllocator.free(newtask.?) catch {};
		}
	}
	return ipcfn.ipc_msg.ENODEST;
}

fn wait_for_process(ptr: *ipcfn.ipc_msg.ipc_message_t) ret_t {
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
	const meself = sch.current_process.?;
	const to_wait = meself.get_port(ptr.dest);
	if(to_wait != null and to_wait.?.owner.load(.seq_cst) != meself) {
		sch.remove_task_from_list(meself,meself.current_queue.?);
		to_wait.?.enqueueWaiter(meself);
		sch.schedule(false);
		return ipcfn.ipc_msg.EOK;
	} else {
		return ipcfn.ipc_msg.ENODEST;
	}
}

fn kill_process(ptr: *const ipcfn.ipc_msg.ipc_message_t) ret_t {
	const mycpuid = main.mycpuid();
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(mycpuid);
	const meself = sch.current_process.?;
	const kill_port = meself.get_port(ptr.dest);
	if(kill_port) |kp| {
		const owner = kp.owner.load(.seq_cst);
		if(owner == null) return ipcfn.ipc_msg.ENODEST;
		if(kp.owner.load(.seq_cst) == meself or kp.rights_mask.load(.seq_cst) & port.Rights.KILL != 0) {
			if(owner.?.cpu_owner == mycpuid) {
				sch.exit(owner.?);
			} else {
				ipi.IPIProtocolHandler.send_ipi(owner.?.cpu_owner,
					ipi.IPIProtocolPayload.init_with_data(
						ipi.IPIProtocolMessageType.EXIT_TASK,
						@intFromPtr(owner.?), 0, 0
					)
				);
			}
		} else {
			return ipcfn.ipc_msg.ENOPERM;
		}
		return ipcfn.ipc_msg.EOK;
	}
	return ipcfn.ipc_msg.ENODEST;
}

fn register_interrupt(ptr: *const ipcfn.ipc_msg.ipc_message_t) ret_t {
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
	sch.current_process.?.register_for_irq(ptr.dest, @truncate(ptr.value0)) catch return ipcfn.ipc_msg.ENODEST;
	return ipcfn.ipc_msg.EOK;
}

fn register_syscall(ptr: *const ipcfn.ipc_msg.ipc_message_t) ret_t {
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
	sch.current_process.?.syscall_port = ptr.dest;
	return ipcfn.ipc_msg.EOK;
}

fn register_fault_handler(ptr: *const ipcfn.ipc_msg.ipc_message_t) ret_t {
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
	sch.current_process.?.fault_port = ptr.dest;
	return ipcfn.ipc_msg.EOK;
}

fn ack_interrupt() ret_t {
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
	const meself = sch.current_process.?;
	if(meself.irq_registered != 0) {
		inttable.acknowledge_interrupt(meself.irq_registered);
		return ipcfn.ipc_msg.EOK;
	} else {
		return ipcfn.ipc_msg.ENOPERM;
	}
}

fn destroy_port(ptr: *const ipcfn.ipc_msg.ipc_message_t) ret_t {
	ipcfn.port_close(schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid()).current_process.?, ptr.dest)
		catch return ipcfn.ipc_msg.ENODEST;
	return ipcfn.ipc_msg.EOK;
}

fn port_io(ptr: *const ipcfn.ipc_msg.ipc_message_t) ret_t {
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
	const meself = sch.current_process.?;
	if(meself.iopb_bitmap == null) {
		meself.iopb_bitmap = iob.IOBufferAllocator.alloc() orelse return ipcfn.ipc_msg.ENODEST;
		@memset(meself.iopb_bitmap.?, 0xFF);
	}
	if(ptr.value0 >= 65536) return ipcfn.ipc_msg.ENODEST;

	const byte = ptr.value0 / 8;
	const bit: u3 = @truncate(ptr.value0 % 8);

	meself.iopb_bitmap.?[byte] &= @as(u8, ~(@as(u8, 1) << bit));
	return ipcfn.ipc_msg.EOK;
}

fn grant_rights(ptr: *const ipcfn.ipc_msg.ipc_message_t) ret_t {
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
	const meself = sch.current_process.?;
	const prt = meself.get_port(ptr.dest);
	if(prt) |p| {
		if(meself == p.owner.load(.seq_cst)) {
			for(0..8) |i| {
				const bit: u8 = @as(u8, 1) << @truncate(i);
				if(ptr.value0 & bit != 0) {
					_ = p.rights_mask.fetchOr(bit, .seq_cst);
				}
			}
			return ipcfn.ipc_msg.EOK;
		} else {
			return ipcfn.ipc_msg.ENOPERM;
		}
	} else {
		return ipcfn.ipc_msg.ENODEST;
	}
}

fn revoke_rights(ptr: *const ipcfn.ipc_msg.ipc_message_t) ret_t {
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
	const meself = sch.current_process.?;
	const prt = meself.get_port(ptr.dest);
	if(prt) |p| {
		if(meself == p.owner.load(.seq_cst)) {
			for(0..8) |i| {
				const bit: u8 = @as(u8, 1) << @truncate(i);
				if(ptr.value0 & bit != 0) {
					_ = p.rights_mask.fetchAnd(~bit, .seq_cst);
				}
			}
			return ipcfn.ipc_msg.EOK;
		} else {
			return ipcfn.ipc_msg.ENOPERM;
		}
	} else {
		return ipcfn.ipc_msg.ENODEST;
	}
}

fn sleep(ptr: *const ipcfn.ipc_msg.ipc_message_t) ret_t {
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
	const meself = sch.current_process.?;
	const prt = meself.get_port(ptr.dest);
	if(prt) |p| {
		const owner = p.owner.load(.seq_cst);
		if(meself == owner or p.rights_mask.load(.seq_cst) & port.Rights.KILL != 0) {
			sch.sleep(ptr.value0, owner.?);
			return ipcfn.ipc_msg.EOK;
		} else {
			return ipcfn.ipc_msg.ENOPERM;
		}
	} else {
		return ipcfn.ipc_msg.ENODEST;
	}
}

fn block(ptr: *const ipcfn.ipc_msg.ipc_message_t) ret_t {
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
	const meself = sch.current_process.?;
	const prt = meself.get_port(ptr.dest);
	if(prt) |p| {
		const owner = p.owner.load(.seq_cst);
		if(owner == null) return ipcfn.ipc_msg.ENODEST; 
		if(meself == owner or p.rights_mask.load(.seq_cst) & port.Rights.KILL != 0) {
			if(owner.?.cpu_owner == meself.cpu_owner) {
				sch.block(owner.?, task.TaskStatus.BLOCKED);
			} else {
				ipi.IPIProtocolHandler.send_ipi(owner.?.cpu_owner, 
				ipi.IPIProtocolPayload.init_with_data(ipi.IPIProtocolMessageType.BLOCK_TASK, 
				@intFromPtr(owner), 0,0));
			}
			return ipcfn.ipc_msg.EOK;
		} else {
			return ipcfn.ipc_msg.ENOPERM;
		}
	} else {
		return ipcfn.ipc_msg.ENODEST;
	}
}

fn unblock(ptr: *const ipcfn.ipc_msg.ipc_message_t) ret_t {
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
	const meself = sch.current_process.?;
	const prt = meself.get_port(ptr.dest);
	if(prt) |p| {
		const owner = p.owner.load(.seq_cst);
		if(owner == null) return ipcfn.ipc_msg.ENODEST; 
		if(meself == owner or p.rights_mask.load(.seq_cst) & port.Rights.KILL != 0) {
			if(owner.?.cpu_owner == meself.cpu_owner) {
				sch.unblock(owner.?);
			} else {
				ipi.IPIProtocolHandler.send_ipi(owner.?.cpu_owner, 
				ipi.IPIProtocolPayload.init_with_data(ipi.IPIProtocolMessageType.UNBLOCK_TASK, 
				@intFromPtr(owner), 0,0));
			}
			return ipcfn.ipc_msg.EOK;
		} else {
			return ipcfn.ipc_msg.ENOPERM;
		}
	} else {
		return ipcfn.ipc_msg.ENODEST;
	}
}

fn start_process(ptr: *const ipcfn.ipc_msg.ipc_message_t) ret_t {
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
	const meself = sch.current_process.?;
	const prt = meself.get_port(ptr.dest);
	if(prt) |p| {
		const owner = p.owner.load(.seq_cst);
		if(owner == null) return ipcfn.ipc_msg.ENODEST;
		if(owner.?.state != task.TaskStatus.STARTING) return ipcfn.ipc_msg.ENOPERM;  
		if(meself == owner or p.rights_mask.load(.seq_cst) & port.Rights.KILL != 0) {
			owner.?.start_user_task(
				@ptrFromInt(ptr.value0),
				@ptrFromInt(ptr.value1),
				task.tls_registers.init(ptr.value2, ptr.value3)
			) catch {
				return ipcfn.ipc_msg.ENODEST;
			};
			ipi.IPIProtocolHandler.send_ipi(owner.?.cpu_owner, 
				ipi.IPIProtocolPayload.init_with_data(ipi.IPIProtocolMessageType.UNBLOCK_TASK, 
				@intFromPtr(owner), 0,0));
			return ipcfn.ipc_msg.EOK;
		} else {
			return ipcfn.ipc_msg.ENOPERM;
		}
	} else {
		return ipcfn.ipc_msg.ENODEST;
	}
}

fn new_addr_space(ptr: *const ipcfn.ipc_msg.ipc_message_t) ret_t {
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
	const meself = sch.current_process.?;
	const prt = meself.get_port(ptr.dest);
	if(prt) |p| {
		const owner = p.owner.load(.seq_cst);
		if(owner == null) return ipcfn.ipc_msg.ENODEST;
		if(meself == owner or p.rights_mask.load(.seq_cst) & port.Rights.KILL != 0) {
			const adr = asa.AddressSpaceAllocator.alloc();
			if(adr) |a| {
				a.* = as.AddressSpace.init(main.km.pm);
				p.target = @ptrCast(a);
				return ipcfn.ipc_msg.EOK;
			} else {
				return ipcfn.ipc_msg.ENODEST;
			}
		} else {
			return ipcfn.ipc_msg.ENOPERM;
		}
	} else {
		return ipcfn.ipc_msg.ENODEST;
	}
}

fn bind_addr_space(ptr: *const ipcfn.ipc_msg.ipc_message_t) ret_t {
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
	const meself = sch.current_process.?;
	const prt = meself.get_port(ptr.dest);
	if(prt) |p| {
		const owner = p.owner.load(.seq_cst);
		if(owner == null) return ipcfn.ipc_msg.ENODEST;
		if(p.target == null) return ipcfn.ipc_msg.ENODEST;
		if(meself == owner or p.rights_mask.load(.seq_cst) & port.Rights.KILL != 0) {
			if(p.target) |a| {
				const addrspacec: *as.AddressSpace = @ptrCast(@alignCast(a));
				owner.?.root_page_table = @ptrFromInt(@intFromPtr(addrspacec.cr3.?) - main.km.hhdm_offset);
				_ = addrspacec.refcount.fetchAdd(1, .acq_rel);
				owner.?.addr_space = addrspacec;
				return ipcfn.ipc_msg.EOK;
			} else {
				return ipcfn.ipc_msg.ENODEST;
			}
		} else {
			return ipcfn.ipc_msg.ENOPERM;
		}
	} else {
		return ipcfn.ipc_msg.ENODEST;
	}
}

fn is_valid_mapping(opts: usize) bool {
	const mask = ipcfn.ipc_msg.PAGE_MAP_CACHE_UNCACHEABLE | ipcfn.ipc_msg.PAGE_MAP_CACHE_WRITE_THROUGH | ipcfn.ipc_msg.PAGE_MAP_CACHE_WRITE_COMBINE;
	const masked = opts & mask;
	return masked == 0 or (masked & (masked - 1)) == 0;
}

fn cache_mapping_to_index(mapping: usize) u3 {
	const mask = ipcfn.ipc_msg.PAGE_MAP_CACHE_UNCACHEABLE | ipcfn.ipc_msg.PAGE_MAP_CACHE_WRITE_THROUGH | ipcfn.ipc_msg.PAGE_MAP_CACHE_WRITE_COMBINE;
	const masked = mapping & mask;
	if(masked & ipcfn.ipc_msg.PAGE_MAP_CACHE_UNCACHEABLE != 0) {
		return 3;
	} else if(masked & ipcfn.ipc_msg.PAGE_MAP_CACHE_WRITE_THROUGH != 0) {
		return 1;
	} else if(masked & ipcfn.ipc_msg.PAGE_MAP_CACHE_WRITE_COMBINE != 0) {
		return 2;
	}
	return 0;
}

fn options(value0: usize) struct{
	o1: page.page_t,
	o2: page.pml2_t,
	o3: page.pml3_t,
	o4: page.pml4_t,
} {
	const index_val = cache_mapping_to_index(value0);
	const rw: u1 = @intFromBool(value0 & ipcfn.ipc_msg.PAGE_MAP_WRITE != 0);
	const us: u1 = @intFromBool(value0 & ipcfn.ipc_msg.PAGE_MAP_USER != 0);
	const xd = @intFromBool(!(value0 & ipcfn.ipc_msg.PAGE_MAP_EXECUTE != 0));
	const opts_l1 = page.page_t{
		.rw = rw,
		.us = us,
		.xd = xd,
		.pcd = @truncate(index_val >> 1),
		.pwt = @truncate(index_val)
	};
	const opts_l2 = page.pml2_t {
		.rw = rw,
		.us = us,
		.xd = xd
	};
	const opts_l3 = page.pml3_t {
		.rw = rw,
		.us = us,
		.xd = xd
	};
	const opts_l4 = page.pml4_t {
		.rw = rw,
		.us = us,
		.xd = xd
	};
	return .{
		.o1 = opts_l1,
		.o2 = opts_l2,
		.o3 = opts_l3,
		.o4 = opts_l4,
	};
}

fn change_options(ptr: *const ipcfn.ipc_msg.ipc_message_t) ret_t {
	if(!is_valid_mapping(ptr.value0)) return ipcfn.ipc_msg.EINVALOP;
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
	const meself = sch.current_process.?;
	const prt = meself.get_port(ptr.dest);
	if(prt) |p| {
		const owner = p.owner.load(.seq_cst);
		if(owner == null) return ipcfn.ipc_msg.ENODEST;
		if(meself == owner) {
			const orig = meself.addr_space.?;
			const opts = options(ptr.value0);
			for(0..ptr.npages) |i| {
				const virtaddr = ptr.page + page.PAGE_SIZE*i;
				if(virtaddr < main.km.pm.hhdm_offset) {
					if(main.km.pm.is_mapped(orig.cr3.?, virtaddr)) {
						orig.add_mapping_4k(null, virtaddr, opts.o1, opts.o2, opts.o3, opts.o4)
						catch return ipcfn.ipc_msg.ENOPERM;
					} else {
						continue;
					}
				} else {
					return ipcfn.ipc_msg.ENOPERM;
				}
			}			
			return ipcfn.ipc_msg.EOK;
		} else {
			return ipcfn.ipc_msg.ENOPERM;
		}
	} else {
		return ipcfn.ipc_msg.ENODEST;
	}
}

fn transfer_page_range(ptr: *const ipcfn.ipc_msg.ipc_message_t) ret_t {
	if(!is_valid_mapping(ptr.value0)) return ipcfn.ipc_msg.EINVALOP;
	if(ptr.page & (page.PAGE_SIZE-1) != 0) return ipcfn.ipc_msg.EINVALOP;
	if(ptr.value1 & (page.PAGE_SIZE-1) != 0) return ipcfn.ipc_msg.EINVALOP;
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
	const meself = sch.current_process.?;
	const prt = meself.get_port(ptr.dest);
	if(prt) |p| {
		const owner = p.owner.load(.seq_cst);
		if(owner == null) return ipcfn.ipc_msg.ENODEST;
		// In this case it's != instead of == because I cannot transfer pages to myself.
		if(meself != owner and p.rights_mask.load(.seq_cst) & port.Rights.KILL != 0) {
			const opts = options(ptr.value0);
			// Check that all are transferrable
			for(0..ptr.npages) |i| {
				const virtaddr = ptr.page + i*page.PAGE_SIZE;
				const physaddr = main.km.pm.get_physaddr(meself.addr_space.?.cr3.?, virtaddr) catch return ipcfn.ipc_msg.ENODEST;
				// Check that the page is not shared and that I'm the owner
				if(pp.PhysicalPageManager.get(physaddr)) |phyp| {
					if(phyp.refcount.load(.acquire) != 1) return ipcfn.ipc_msg.ENOPERM;
				} else {
					return ipcfn.ipc_msg.ENODEST;
				}
			}
			// Now transfer
			for(0..ptr.npages) |i| {
				const virtaddr_origin = ptr.page + i*page.PAGE_SIZE;
				const virtaddr_dest = ptr.value1 + i*page.PAGE_SIZE;
				const physaddr = main.km.pm.get_physaddr(meself.addr_space.?.cr3.?, virtaddr_origin) catch unreachable;
				const physpage = pp.PhysicalPageManager.get(physaddr) orelse unreachable;
				// Validated that page is not shared last step
				physpage.set_grantor(meself.addr_space.?, virtaddr_origin);
				physpage.set_owner(owner.?.addr_space.?, virtaddr_dest);

				meself.addr_space.?.remove_mapping_4k(virtaddr_origin) catch unreachable;
				owner.?.addr_space.?.add_mapping_4k(physaddr, virtaddr_dest, opts.o1, opts.o2, opts.o3, opts.o4) catch unreachable;
			}
			// Signal page transfer
			return ipcfn.ipc_send(ptr);
		} else {
			return ipcfn.ipc_msg.ENOPERM;
		}
	} else {
		return ipcfn.ipc_msg.ENODEST;
	}
}

fn share_page_range(ptr: *const ipcfn.ipc_msg.ipc_message_t) ret_t {
	if(!is_valid_mapping(ptr.value0)) return ipcfn.ipc_msg.EINVALOP;
	if(ptr.page & (page.PAGE_SIZE-1) != 0) return ipcfn.ipc_msg.EINVALOP;
	if(ptr.value1 & (page.PAGE_SIZE-1) != 0) return ipcfn.ipc_msg.EINVALOP;
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
	const meself = sch.current_process.?;
	const prt = meself.get_port(ptr.dest);
	if(prt) |p| {
		const owner = p.owner.load(.seq_cst);
		if(owner == null) return ipcfn.ipc_msg.ENODEST;
		// In this case it's != instead of == because I cannot share pages to myself.
		if(meself != owner and p.rights_mask.load(.seq_cst) & port.Rights.KILL != 0) {
			const opts = options(ptr.value0);
			// Check that all are transferrable
			for(0..ptr.npages) |i| {
				const virtaddr = ptr.page + i*page.PAGE_SIZE;
				if(!main.km.pm.is_mapped(meself.addr_space.?.cr3.?, virtaddr)) {
					return ipcfn.ipc_msg.ENODEST;
				}
			}
			// Now transfer
			for(0..ptr.npages) |i| {
				const virtaddr_origin = ptr.page + i*page.PAGE_SIZE;
				const virtaddr_dest = ptr.value1 + i*page.PAGE_SIZE;
				const physaddr = main.km.pm.get_physaddr(meself.addr_space.?.cr3.?, virtaddr_origin) catch unreachable;
				const ppdata = pp.PhysicalPageManager.get(physaddr) orelse unreachable;
				ppdata.add_addr_space(owner.?.addr_space.?, virtaddr_dest) catch unreachable;
				owner.?.addr_space.?.add_mapping_4k(physaddr, virtaddr_dest, opts.o1, opts.o2, opts.o3, opts.o4) catch unreachable;
			}
			// Signal page transfer
			return ipcfn.ipc_send(ptr);
		} else {
			return ipcfn.ipc_msg.ENOPERM;
		}
	} else {
		return ipcfn.ipc_msg.ENODEST;
	}
}

fn revoke_page_range(ptr: *const ipcfn.ipc_msg.ipc_message_t) ret_t {
	if(!is_valid_mapping(ptr.value0)) return ipcfn.ipc_msg.EINVALOP;
	if(ptr.page & (page.PAGE_SIZE-1) != 0) return ipcfn.ipc_msg.EINVALOP;
	if(ptr.value1 & (page.PAGE_SIZE-1) != 0) return ipcfn.ipc_msg.EINVALOP;
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
	const meself = sch.current_process.?;
	const prt = meself.get_port(ptr.dest);
	if(prt) |p| {
		const owner = p.owner.load(.seq_cst);
		if(owner == null) return ipcfn.ipc_msg.ENODEST;
		// In this case it's != instead of == because I cannot transfer pages to myself.
		if(meself != owner and p.rights_mask.load(.seq_cst) & port.Rights.KILL != 0) {
			const opts = options(ptr.value0);
			// Check that all are transferrable
			for(0..ptr.npages) |i| {
				const virtaddr = ptr.page + i*page.PAGE_SIZE;
				const physaddr = main.km.pm.get_physaddr(owner.?.addr_space.?.cr3.?, virtaddr) catch return ipcfn.ipc_msg.ENODEST;
				// Check that the page is not shared and that I'm the granter
				if(pp.PhysicalPageManager.get(physaddr)) |phyp| {
					if(phyp.grantor.owner != meself.addr_space.?) return ipcfn.ipc_msg.ENOPERM;
				} else {
					return ipcfn.ipc_msg.ENODEST;
				}
			}
			// Now transfer
			for(0..ptr.npages) |i| {
				const virtaddr_origin = ptr.page + i*page.PAGE_SIZE;
				const virtaddr_dest = ptr.value1 + i*page.PAGE_SIZE;
				const physaddr = main.km.pm.get_physaddr(owner.?.addr_space.?.cr3.?, virtaddr_origin) catch unreachable;
				const physpage = pp.PhysicalPageManager.get(physaddr) orelse unreachable;
				// Validated that page is not shared last step
				physpage.grantor.owner = null;
				physpage.set_owner(meself.addr_space.?, virtaddr_dest);

				owner.?.addr_space.?.remove_mapping_4k(virtaddr_origin) catch unreachable;
				meself.addr_space.?.add_mapping_4k(physaddr, virtaddr_dest, opts.o1, opts.o2, opts.o3, opts.o4) catch unreachable;
			}
			// Signal page transfer
			return ipcfn.ipc_send(ptr);
		} else {
			return ipcfn.ipc_msg.ENOPERM;
		}
	} else {
		return ipcfn.ipc_msg.ENODEST;
	}
}

pub export fn syscall_handler_stage_1(
	rdi: u64,
	rsi: u64,
	rdx: u64,
	r10: u64,
	r8: u64,
	r9: u64,
	rax: u64,
) callconv(.C) ret_t {
	std.log.info("sys: {} {} {} {} {} {} {}", .{rax, rdi, rsi, rdx, r10, r8, r9});
	switch(rax) {
		// ipc_send
		std.math.maxInt(u64) => {
			if(rdi == 0 or rdi > main.km.hhdm_offset or rdi + @sizeOf(ipcfn.ipc_msg.ipc_message_t) > main.km.hhdm_offset) return ipcfn.ipc_msg.EINVALMSG;
			const ptr: *const ipcfn.ipc_msg.ipc_message_t = @ptrFromInt(rdi);
			return switch(ptr.flags) {
				0, ipcfn.ipc_msg.IPC_FLAG_NONBLOCKING, ipcfn.ipc_msg.IPC_FLAG_TRANSFER_PORT => @intCast(ipcfn.ipc_send(ptr)),
				ipcfn.ipc_msg.IPC_FLAG_KILL_PROCESS => kill_process(ptr),
				ipcfn.ipc_msg.IPC_FLAG_INT_REGISTER => register_interrupt(ptr),
				ipcfn.ipc_msg.IPC_FLAG_SYSCALL_REGISTER => register_syscall(ptr),
				ipcfn.ipc_msg.IPC_FLAG_INT_ACK => ack_interrupt(),
				ipcfn.ipc_msg.IPC_FLAG_DESTROY_PORT => destroy_port(ptr),
				ipcfn.ipc_msg.IPC_FLAG_FAULT_HANDLER => register_fault_handler(ptr),
				ipcfn.ipc_msg.IPC_FLAG_PORT_IO => port_io(ptr),
				ipcfn.ipc_msg.IPC_FLAG_PROCESS_SLEEP => sleep(ptr),
				ipcfn.ipc_msg.IPC_FLAG_PROCESS_BLOCK => block(ptr),
				ipcfn.ipc_msg.IPC_FLAG_PROCESS_UNBLOCK => unblock(ptr),
				ipcfn.ipc_msg.IPC_FLAG_GRANT_RIGHTS => grant_rights(ptr),
				ipcfn.ipc_msg.IPC_FLAG_REVOKE_RIGHTS => revoke_rights(ptr),
				ipcfn.ipc_msg.IPC_FLAG_NEW_ADDR_SPACE_PROC => new_addr_space(ptr),
				ipcfn.ipc_msg.IPC_FLAG_BIND_ADDR_SPACE => bind_addr_space(ptr),
				ipcfn.ipc_msg.IPC_FLAG_START_PROCESS => start_process(ptr),
				
				ipcfn.ipc_msg.IPC_FLAG_MAP_PAGE => change_options(ptr),
				ipcfn.ipc_msg.IPC_FLAG_GRANT_PAGE => transfer_page_range(ptr),
				ipcfn.ipc_msg.IPC_FLAG_SHARE_PAGE => share_page_range(ptr),
				ipcfn.ipc_msg.IPC_FLAG_REVOKE_PAGE => revoke_page_range(ptr),

				else => ipcfn.ipc_msg.EINVALOP
			};
		},
		// ipc_recv
		std.math.maxInt(u64) - 1 => {
			if(rdi == 0 or rdi > main.km.hhdm_offset or rdi + @sizeOf(ipcfn.ipc_msg.ipc_message_t) > main.km.hhdm_offset) return ipcfn.ipc_msg.EINVALMSG;
			const ptr: *ipcfn.ipc_msg.ipc_message_t = @ptrFromInt(rdi);
			return switch(ptr.flags) {
				0, ipcfn.ipc_msg.IPC_FLAG_NONBLOCKING, ipcfn.ipc_msg.IPC_FLAG_TRANSFER_PORT => @intCast(ipcfn.ipc_recv(ptr)),
				ipcfn.ipc_msg.IPC_FLAG_CREATE_PORT => create_port(ptr),
				ipcfn.ipc_msg.IPC_FLAG_CREATE_PROCESS => create_process(ptr),
				ipcfn.ipc_msg.IPC_FLAG_WAIT_PROCESS => wait_for_process(ptr),
				else => ipcfn.ipc_msg.EINVALOP
			};
		},
		else => {
			const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
			const meself = sch.current_process.?;
			if(meself.syscall_port != -1) {
				var msg = ipcfn.ipc_msg.ipc_message_t {
					.dest = meself.syscall_port,
					.source =  0,
					.value0 = rax,
					.value1 = rdi,
					.value2 = rsi,
					.value3 = rdx,
					.value4 = r10,
					.page = r8,
					.npages = r9,
					.flags = 0
				};
				const send = ipcfn.ipc_send(&msg);
				if(send != 0) return send;
				const recv = ipcfn.ipc_recv(&msg);
				if(recv != 0) return recv;
				return @bitCast(msg.value0);
			}
			// No syscall server registered then -1
			return -1;
		}
	}
}


pub fn inner_transfer_page_range(ptr: *const ipcfn.ipc_msg.ipc_message_t, srcaddrspace: *as.AddressSpace) ret_t {
	if(!is_valid_mapping(ptr.value0)) return ipcfn.ipc_msg.EINVALOP;
	if(ptr.page & (page.PAGE_SIZE-1) != 0) return ipcfn.ipc_msg.EINVALOP;
	if(ptr.value1 & (page.PAGE_SIZE-1) != 0) return ipcfn.ipc_msg.EINVALOP;
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
	const prt = sch.current_process.?.get_port(ptr.dest);
	if(prt) |p| {
		const owner = p.owner.load(.seq_cst);
		if(owner == null) @panic("MEMORY MANAGER DOES NOT EXIST");
		// In this case it's != instead of == because I cannot transfer pages to myself.
		const opts = options(ptr.value0);
		// Transfer
		for(0..ptr.npages) |i| {
			const virtaddr_origin = ptr.page + i*page.PAGE_SIZE;
			const virtaddr_dest = ptr.value1 + i*page.PAGE_SIZE;
			const physaddr = main.km.pm.get_physaddr(srcaddrspace.cr3.?, virtaddr_origin) catch unreachable;
			const physpage = pp.PhysicalPageManager.get(physaddr) orelse unreachable;
			// Validated that page is not shared last step
			physpage.set_grantor(srcaddrspace, virtaddr_origin);
			physpage.set_owner(owner.?.addr_space.?, virtaddr_dest);

			srcaddrspace.remove_mapping_4k(virtaddr_origin) catch unreachable;
			owner.?.addr_space.?.add_mapping_4k(physaddr, virtaddr_dest, opts.o1, opts.o2, opts.o3, opts.o4) catch unreachable;
		}
		// Signal page transfer
		return ipcfn.ipc_send(ptr);
	} else {
		return ipcfn.ipc_msg.ENODEST;
	}
}
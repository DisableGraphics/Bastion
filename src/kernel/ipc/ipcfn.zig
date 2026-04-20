pub const ipc_msg = @cImport(
	@cInclude("ipc.h")	
);
const schman = @import("../scheduler/manager.zig");
const main = @import("../main.zig");
const port = @import("port.zig");
const assm = @import("../arch/x86_64/asm.zig");
const std = @import("std");
const tsk = @import("../scheduler/task.zig");
const sched = @import("../scheduler/scheduler.zig");
const ipi = @import("../interrupts/ipi_protocol.zig");
const porta = @import("portalloc.zig");
const spin = @import("../sync/spinlock.zig");

fn wake_task(task: *tsk.Task, sch: ?*sched.Scheduler, cpuid: u32) void {
	if(task.cpu_owner == cpuid) {
		if(sch) |s| {
			s.add_task_with_lock_held(task);
		} else {
			var s = schman.SchedulerManager.get_scheduler_for_cpu(cpuid);
			s.add_task_with_lock_held(task);
		}
	} else {
		// Interrupt thread task -> can't be scheduled
		if(task.cpu_owner == std.math.maxInt(u32)) {
			task.next = null;
			task.prev = null;
			return;
		}
		ipi.IPIProtocolHandler.send_ipi(task.cpu_owner, 
			ipi.IPIProtocolPayload.init_with_data(
			ipi.IPIProtocolMessageType.WAKE_TASK_IPC, 
			@intFromPtr(task), 0, 0)
		);
	}
}

fn start_task(task: *tsk.Task, sch: ?*sched.Scheduler, cpuid: u32) void {
	if(task.cpu_owner == cpuid) {
		if(sch) |s| {
			s.schedule_to(task);
		} else {
			var s = schman.SchedulerManager.get_scheduler_for_cpu(cpuid);
			s.schedule_to(task);
		}
	}
}

fn transfer_message(
    src: *const ipc_msg.ipc_message_t,
    dst: *ipc_msg.ipc_message_t,
) void {
    dst.flags = src.flags;
    dst.value0 = src.value0;
	dst.value1 = src.value1;
	dst.value2 = src.value2;
	dst.value3 = src.value3;
	dst.value4 = src.value4;
    dst.npages = src.npages;
    dst.page = src.page;
}

pub fn port_create(task: *tsk.Task) ?*port.Port {
	const p = porta.PortAllocator.alloc() orelse return null;
	p.rights_mask.store(port.Rights.SEND, .seq_cst);
	p.owner.store(task, .seq_cst);
	p.cpu_owner.store(@truncate(main.mycpuid()), .release);
	return p;
}

fn wake_all(prt: *port.Port, sch: *sched.Scheduler, cpuid: u32) void {
	// Wake up all receivers
	while(prt.dequeueReceiver()) |recv| {
		wake_task(recv, sch, cpuid);
	}
	// Wake up all receivers
	while(prt.dequeueSender()) |send| {
		wake_task(send, sch, cpuid);
	}
	// Wake up all waiters
	while(prt.dequeueWaiter()) |wait| {
		wake_task(wait, sch, cpuid);
	}
}

pub fn port_close(task: *tsk.Task, prt: i16) !void {
	const ptr = task.close_port(prt) orelse return error.NOT_FOUND;
	const flags = ptr.lock.lock();
	const mycpuid = main.mycpuid();
	defer ptr.lock.unlock(flags);
	if(ptr.owner.load(.acquire) == task) {
		ptr.owner.store(null, .release);
		wake_all(ptr, schman.SchedulerManager.get_scheduler_for_cpu(mycpuid), @truncate(mycpuid));
	}
	const count = ptr.count.load(.seq_cst);
	if(count == 0) {
		const owner = ptr.cpu_owner.load(.acquire);
		if(owner == mycpuid) {
			try porta.PortAllocator.free(ptr);
		} else {
			ipi.IPIProtocolHandler.send_ipi(owner,
				ipi.IPIProtocolPayload.init_with_data(
					ipi.IPIProtocolMessageType.FREE_PORT,
					@intFromPtr(ptr), 0, 0
				)
			);
		}
	}
}

fn port_copy(prt: *port.Port, src_task: *tsk.Task, dst_task: *tsk.Task) !i16 {
	if(prt.owner.load(.acquire) == src_task or prt.rights_mask.load(.acquire) & port.Rights.TRANSF != 0) {
		return try dst_task.add_port(prt);
	}
	return error.NO_PERMISSIONS;
}

pub fn ipc_send(msg: ?*const ipc_msg.ipc_message_t) i32 {
	if (msg == null) {
		return ipc_msg.EINVALMSG;
	}
	
	const m = msg.?;
	const dest_port = m.dest;
	const mycpu = main.mycpuid();
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(mycpu);
	const init_flags = assm.irqdisable();

	const this = sch.current_process.?;
	const dstport = this.get_port(dest_port);
	if(dstport == null) {
		assm.irqrestore(init_flags);
		return ipc_msg.ENODEST;
	}
	assm.irqrestore(init_flags);
	const flags = dstport.?.lock.lock();

	// Permission check
	const owner = dstport.?.owner.load(.acquire);
	const dst_rights = dstport.?.rights_mask.load(.acquire);
	const can_send = (this == owner) or ((dst_rights & port.Rights.SEND) != 0);
	if(!can_send) {
		dstport.?.lock.unlock(flags);
		return ipc_msg.ENOPERM;
	}
	
	const q = dstport.?.dequeueReceiver();
	var retvalue = ipc_msg.EOK;
	if(q) |receiver| {
		if(m.flags & ipc_msg.IPC_FLAG_TRANSFER_PORT != 0) {
			const src_port = m.source;
			const srcport = this.get_port(src_port);
			if(srcport == null) {
				dstport.?.lock.unlock(flags);
				return ipc_msg.ENODEST;
			}
			const v = port_copy(srcport.?, this, receiver);
			if(v) |portval| {
				receiver.receive_msg.?.source = portval;
			} else |_| {
				retvalue = ipc_msg.ENOPERM;
			}
		}
		transfer_message(m, receiver.receive_msg.?);
		wake_task(receiver, sch, @truncate(mycpu));
		dstport.?.lock.unlock(flags);
		start_task(receiver, sch, @truncate(mycpu));
		return retvalue;
	} else {
		if(msg.?.flags & ipc_msg.IPC_FLAG_NONBLOCKING == 0) {
			this.send_msg = m;
			sch.remove_task_from_list(this, this.current_queue.?);
			dstport.?.enqueueSender(this);
			dstport.?.lock.unlock(flags);
			sch.schedule_with_lock_held(false);
			if(dstport.?.owner.load(.acquire) == null) {
				retvalue = ipc_msg.ENOOWN;
			}
		} else {
			dstport.?.lock.unlock(flags);
			retvalue = ipc_msg.ENODEST;
		}
		
		return retvalue;
	}
}

pub fn ipc_recv(msg: ?*ipc_msg.ipc_message_t) i32 {
	if (msg == null) {
		return ipc_msg.EINVALMSG;
	}

	const m = msg.?;
	const dest_port = m.dest;
	const mycpu = main.mycpuid();
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(mycpu);
	const init_flags = assm.irqdisable();

	const this = sch.current_process.?;
	const dstport = this.get_port(dest_port);
	if (dstport == null) {
		assm.irqrestore(init_flags);
		return ipc_msg.ENODEST;
	}
	assm.irqrestore(init_flags);
	const flags = dstport.?.lock.lock();

	// Permission check
	const owner = dstport.?.owner.load(.acquire);
	const rights = dstport.?.rights_mask.load(.acquire);
	const can_recv = (this == owner) or ((rights & port.Rights.RECV) != 0);
	if (!can_recv) {
		dstport.?.lock.unlock(flags);
		return ipc_msg.ENOPERM;
	}
	
	const q = dstport.?.dequeueSender();
	var retvalue = ipc_msg.EOK;
	if (q) |sender| {
		if(sender.send_msg.?.flags & ipc_msg.IPC_FLAG_TRANSFER_PORT != 0) {
			const po = sender.get_port(sender.send_msg.?.source);
			if(po != null) {
				const v = port_copy(po.?, sender, this);
				if(v) |portval| {
					m.source = portval;
				} else |_| {
					retvalue = ipc_msg.ENOPERM;
				}
			} else {
				retvalue = ipc_msg.ENODEST;
			}
		}
		transfer_message(sender.send_msg.?, m);
		wake_task(sender, sch, @truncate(mycpu));
		dstport.?.lock.unlock(flags);
		start_task(sender, sch, @truncate(mycpu));
		return retvalue;
	} else {
		if(msg.?.flags & ipc_msg.IPC_FLAG_NONBLOCKING == 0) {
			this.receive_msg = msg;
			sch.remove_task_from_list(this, this.current_queue.?);
			dstport.?.enqueueReceiver(this);
			dstport.?.lock.unlock(flags);
			sch.schedule_with_lock_held(false);
			if(dstport.?.owner.load(.acquire) == null) {
				retvalue = ipc_msg.ENOOWN;
			}
		} else {
			dstport.?.lock.unlock(flags);
			retvalue = ipc_msg.ENODEST;
		}
		return retvalue;
	}
}

pub fn ipc_send_from_irq(msg: ipc_msg.ipc_message_t, this: *tsk.Task) i32 {
	const dest_port = msg.dest;
	const dstport = this.get_port(dest_port);
	if(dstport == null) {
		return ipc_msg.ENODEST;
	}
	const flags = dstport.?.lock.lock();
	// As I'm an interrupt thread I don't need permission checks and most fluff 
	const q = dstport.?.dequeueReceiver();
	const retvalue = ipc_msg.EOK;
	if(q) |wr| {
		transfer_message(&msg, wr.receive_msg.?);
		wake_task(wr, null, @truncate(main.mycpuid()));
		dstport.?.lock.unlock(flags);
		start_task(wr, null, @truncate(main.mycpuid()));
		return retvalue;
	} else {
		this.send_msg = &msg;
		dstport.?.enqueueSender(this);
		dstport.?.lock.unlock(flags);
		return retvalue;
	}
}
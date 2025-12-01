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
		if(sch != null) {
			sch.?.add_task_to_list(task, &sch.?.queues[0]);
		} else {
			var s = schman.SchedulerManager.get_scheduler_for_cpu(cpuid);
			s.add_task_to_list(task, &s.queues[0]);
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
			ipi.IPIProtocolMessageType.TASK_LOAD_BALANCING_RESPONSE, 
			@intFromPtr(task), 0, 0)
		);
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
}

pub fn port_close(task: *tsk.Task, prt: i16) !void {
	const ptr = task.close_port(prt) orelse return error.NOT_FOUND;
	ptr.lock.lock();
	const mycpuid = main.mycpuid();
	defer ptr.lock.unlock();
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
	const src_port = m.source;
	const mycpu = main.mycpuid();
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(mycpu);
	sch.lock();
	const this = sch.current_process.?;
	const srcport = this.get_port(src_port);
	const dstport = this.get_port(dest_port);

	dstport.?.lock.lock();
	if(srcport == null or dstport == null) {
		sch.unlock();
		return ipc_msg.ENODEST;
	}
	// permissions correct
	const src_owner = dstport.?.owner.load(.acquire);
	const dst_rights = dstport.?.rights_mask.load(.acquire);

	const can_send = (this == src_owner) or ((dst_rights & port.Rights.SEND) != 0);
	if(!can_send) {
		dstport.?.lock.unlock();
		sch.unlock();
		return ipc_msg.ENOPERM;
	}
	
	const q = dstport.?.dequeueReceiver();
	var retvalue = ipc_msg.EOK;
	if(q) |wr| {
		if(m.flags & ipc_msg.IPC_FLAG_TRANSFER_PORT != 0) {
			const v = port_copy(srcport.?, this, wr);
			if(v) |portval| {
				wr.receive_msg.?.source = portval;
			} else |_| {
				retvalue = ipc_msg.ENOPERM;
			}
		}
		transfer_message(m, wr.receive_msg.?);
		wake_task(wr, sch, @truncate(mycpu));
		dstport.?.lock.unlock();
		sch.unlock();
		return retvalue;
	} else {
		if(msg.?.flags & ipc_msg.IPC_FLAG_NONBLOCKING == 0) {
			this.send_msg = m;
			sch.remove_task_from_list(this, this.current_queue.?);
			dstport.?.enqueueSender(this);
			dstport.?.lock.unlock();
			sch.schedule_with_lock_held(false);
			if(dstport.?.owner.load(.acquire) == null) {
				retvalue = ipc_msg.ENOOWN;
			}
		} else {
			dstport.?.lock.unlock();
			sch.unlock();
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
	const recv_port_id = m.dest;
	const mycpu = main.mycpuid();
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(mycpu);
	sch.lock();

	const this = sch.current_process.?;
	const recv_port = this.get_port(recv_port_id);
	if (recv_port == null) {
		sch.unlock();
		return ipc_msg.ENODEST;
	}
	const flags = irq_lock(&recv_port.?.lock);

	// Permission check
	const owner = recv_port.?.owner.load(.acquire);
	const rights = recv_port.?.rights_mask.load(.acquire);
	const can_recv = (this == owner) or ((rights & port.Rights.RECV) != 0);
	if (!can_recv) {
		irq_unlock(&recv_port.?.lock, flags);
		sch.unlock();
		return ipc_msg.ENOPERM;
	}
	
	const q = recv_port.?.dequeueSender();
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
		irq_unlock(&recv_port.?.lock, flags);
		sch.unlock();
		
		return retvalue;
	} else {
		if(msg.?.flags & ipc_msg.IPC_FLAG_NONBLOCKING == 0) {
			this.receive_msg = msg;
			sch.remove_task_from_list(this, this.current_queue.?);
			recv_port.?.enqueueReceiver(this);
			irq_unlock(&recv_port.?.lock, flags);
			sch.schedule_with_lock_held(false);
			if(recv_port.?.owner.load(.acquire) == null) {
				retvalue = ipc_msg.ENOOWN;
			}
		} else {
			irq_unlock(&recv_port.?.lock, flags);
			sch.unlock();
			retvalue = ipc_msg.ENODEST;
		}
		return retvalue;
	}
}

fn irq_lock(lock: *spin.SpinLock) usize {
	const flags = assm.irqdisable();
	lock.lock();
	return flags;
}

fn irq_unlock(lock: *spin.SpinLock, flags: usize) void {
	lock.unlock();
	assm.irqrestore(flags);
}

pub fn ipc_send_from_irq(msg: ipc_msg.ipc_message_t, this: *tsk.Task) i32 {
	const dest_port = msg.dest;
	const dstport = this.get_port(dest_port);
	const flags = irq_lock(&dstport.?.lock);
	if(dstport == null) {
		return ipc_msg.ENODEST;
	}
	// As I'm an interrupt thread I don't need permission checks and most fluff 
	const q = dstport.?.dequeueReceiver();
	const retvalue = ipc_msg.EOK;
	if(q) |wr| {
		transfer_message(&msg, wr.receive_msg.?);
		wake_task(wr, null, @truncate(main.mycpuid()));
		irq_unlock(&dstport.?.lock, flags);
		return retvalue;
	} else {
		this.send_msg = &msg;
		dstport.?.enqueueSender(this);
		irq_unlock(&dstport.?.lock, flags);
		return retvalue;
	}
}
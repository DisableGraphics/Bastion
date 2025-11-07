pub const ipc_msg = @cImport(
	@cInclude("ipc.h")	
);
const schman = @import("../scheduler/manager.zig");
const main = @import("../main.zig");
const port = @import("port.zig");

pub fn ipc_send(msg: ?*const ipc_msg.ipc_message_t) i32 {
	if (msg == null) {
		return ipc_msg.EINVALMSG;
	}
	const m = msg.?;
	
	const dest_port = m.dest;
	const src_port = m.source;
	const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
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
		sch.unlock();
		return ipc_msg.ENOPERM;
	}
	
	const q = dstport.?.dequeueReceiver();
	if(q) |wr| {
		dstport.?.lock.unlock();
		wr.receive_msg.?.flags = m.flags;
		wr.receive_msg.?.value = m.value;
		wr.receive_msg.?.npages = m.npages;
		wr.receive_msg.?.page = m.page;
		sch.add_task_to_list(wr, &sch.queues[0]);
		sch.unlock();
		return ipc_msg.EOK;
	} else {
		this.send_msg = m;
		sch.remove_task_from_list(this, this.current_queue.?);
		dstport.?.enqueueSender(this);
		dstport.?.lock.unlock();
		sch.unlock();
		sch.schedule(false);
		return ipc_msg.EOK;
	}
}

pub fn ipc_recv(msg: ?*ipc_msg.ipc_message_t) i32 {
    if (msg == null) {
        return ipc_msg.EINVALMSG;
    }

    const m = msg.?;
    const recv_port_id = m.dest; // Usually, the port you're receiving *from*
    const sch = schman.SchedulerManager.get_scheduler_for_cpu(main.mycpuid());
    sch.lock();

    const this = sch.current_process.?;
    const recv_port = this.get_port(recv_port_id);
    if (recv_port == null) {
        sch.unlock();
        return ipc_msg.ENODEST;
    }

	recv_port.?.lock.lock();

    // Permission check — must own the port or have RECV rights
    const owner = recv_port.?.owner.load(.acquire);
    const rights = recv_port.?.rights_mask.load(.acquire);
    const can_recv = (this == owner) or ((rights & port.Rights.RECV) != 0);
    if (!can_recv) {
        sch.unlock();
        return ipc_msg.ENOPERM;
    }
	
    // Try to find a waiting sender
	const q = recv_port.?.dequeueSender();
    if (q) |sender| {
		recv_port.?.lock.unlock();
        // Sender is waiting — deliver message directly
        m.* = sender.send_msg.?.*;
        sch.add_task_to_list(sender, &sch.queues[0]); // wake sender
        sch.unlock();
		
        return ipc_msg.EOK;
    } else {
        // No sender waiting — block receiver
        this.receive_msg = msg; // store where the message should go
        sch.remove_task_from_list(this, this.current_queue.?);
        recv_port.?.enqueueReceiver(this);
		recv_port.?.lock.unlock();
        sch.unlock();
        sch.schedule(false);
        return ipc_msg.EOK;
    }
}

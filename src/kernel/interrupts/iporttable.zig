const port = @import("../ipc/port.zig");
const ipcfn = @import("../ipc/ipcfn.zig");
const schman = @import("../scheduler/manager.zig");
const main = @import("../main.zig");
const tsk = @import("../scheduler/task.zig");
const portall = @import("../ipc/portalloc.zig");
const pic = @import("../arch/x86_64/controllers/pic.zig");
const ipi = @import("ipi_protocol.zig");

const n_ports = 256-32;

pub const InterruptPortTable = struct {
	var ttable: [n_ports]tsk.Task = [_]tsk.Task{tsk.Task.init_ipc_interrupt_task()} ** n_ports;
	pub fn register_irq(prt: *port.Port, irqn: u8) !void {
		// Basically if there is a port registered do not register another
		if(ttable[irqn].get_port(0) == null) {
			_ = try ttable[irqn].add_port(prt);
			acknowledge_interrupt(irqn);
		}
	}
	pub fn unregister_irq(irqn: u8) void {
		if(ttable[irqn].get_port(0) != null) {
			ttable[irqn].close_port(0);
		}
	}
	pub fn notify_interrupt(irqn: u8) void {
		if(ttable[irqn].get_port(0)) |_| {
			// If it's not already pending, try to send the msg
			if(ttable[irqn].next == null and ttable[irqn].prev == null) {
				const cpu = ttable[irqn].get_port(0).?.owner.load(.acquire).?.cpu_owner;
				ipi.IPIProtocolHandler.send_ipi(cpu, ipi.IPIProtocolPayload.init_with_data(
					ipi.IPIProtocolMessageType.SEND_IRQ_MSG, irqn, @intFromPtr(&ttable[irqn]), 0));
				//_ = ipcfn.ipc_send_from_irq(&msg, &ttable[irqn]);
			}
		}
	}
};

pub fn acknowledge_interrupt(irqn: u8) void {
	if(irqn < 16) pic.PIC.enable_irq(@truncate(irqn));
}
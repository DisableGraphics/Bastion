const port = @import("../ipc/port.zig");
const ipcfn = @import("../ipc/ipcfn.zig");
const schman = @import("../scheduler/manager.zig");
const main = @import("../main.zig");
const tsk = @import("../scheduler/task.zig");
const portall = @import("../ipc/portalloc.zig");

const n_ports = 256-32;

const InterruptPortTable = struct {
	var ttable: [n_ports]tsk.Task = [_]tsk.Task{tsk.Task.init_ipc_interrupt_task()} ** n_ports;
	fn register_irq(prt: *port.Port, irqn: u8) void {
		// Basically if there is a port registered do not register another
		if(ttable[irqn].get_port(0) == null) {
			_ = ttable[irqn].add_port(prt);
		}
	}
	fn unregister_irq(irqn: u8) void {
		if(ttable[irqn].get_port(0) != null) {
			ttable[irqn].close_port(0);
		}
	}
	fn notify_interrupt(irqn: u8) !void {
		if(ttable[irqn].get_port(1)) |_| {
			const msg = ipcfn.ipc_msg.ipc_message_t{
				.source = 0,
				.dest = 0,
				.flags = 0,
				.value = irqn,
				.npages = 0,
				.page = 0
			};
			// If it's not already pending, try to send the msg
			if(ttable[irqn].next == null and ttable[irqn].prev == null) {
				ipcfn.ipc_send_from_irq(msg, ttable[irqn]);
			}
		}
	}
};
#include "syswrap.h"

ipc_transfer_t sys_share_page(port_t dest, virtaddr_t page, size_t n, virtaddr_t wheretomap, size_t mapping_options) {
	const ipc_message_t msg = {
		.dest = dest,
		.flags = IPC_FLAG_SHARE_PAGE,
		.npages = n,
		.page = page,
		.value1 = wheretomap,
		.value0 = mapping_options
	};
	return sys_ipc_send(&msg);
}

ipc_transfer_t sys_transfer_page(port_t dest, virtaddr_t page, size_t n, virtaddr_t wheretomap, size_t mapping_options) {
	const ipc_message_t msg = {
		.dest = dest,
		.flags = IPC_FLAG_GRANT_PAGE,
		.npages = n,
		.page = page,
		.value1 = wheretomap,
		.value0 = mapping_options
	};
	return sys_ipc_send(&msg);
}

ipc_transfer_t sys_revoke_page(port_t dest, virtaddr_t page, size_t n, virtaddr_t wheretomap, size_t mapping_options) {
	const ipc_message_t msg = {
		.dest = dest,
		.flags = IPC_FLAG_REVOKE_PAGE,
		.npages = n,
		.page = page,
		.value1 = wheretomap,
		.value0 = mapping_options
	};
	return sys_ipc_send(&msg);
}

ipc_transfer_t sys_map_page(virtaddr_t page, size_t n, size_t mapping_options) {
	const ipc_message_t msg = {
		.flags = IPC_FLAG_REVOKE_PAGE,
		.npages = n,
		.page = page,
		.value0 = mapping_options
	};
	return sys_ipc_send(&msg);
}

//--------- PROCESS MANAGEMENT ----------------
port_t sys_create_process() {
	ipc_message_t msg = {
		.flags = IPC_FLAG_CREATE_PROCESS,
	};
	size_t ret = sys_ipc_recv(&msg);
	if(ret != 0) return ret;
	return msg.value0;
}

result_t sys_kill_process(port_t dest) {
	const ipc_message_t msg = {
		.dest = dest,
		.flags = IPC_FLAG_KILL_PROCESS,
	};
	return sys_ipc_send(&msg);
}

result_t sys_create_address_space(port_t dest) {
	const ipc_message_t msg = {
		.dest = dest,
		.flags = IPC_FLAG_NEW_ADDR_SPACE_PROC,
	};
	return sys_ipc_send(&msg);
}

result_t sys_bind_address_space(port_t dest) {
	const ipc_message_t msg = {
		.dest = dest,
		.flags = IPC_FLAG_BIND_ADDR_SPACE,
	};
	return sys_ipc_send(&msg);
}

result_t sys_wait_process(port_t dest) {
	ipc_message_t msg = {
		.dest = dest,
		.flags = IPC_FLAG_WAIT_PROCESS,
	};
	return sys_ipc_recv(&msg);
}

result_t sys_block_process(port_t dest) {
	const ipc_message_t msg = {
		.dest = dest,
		.flags = IPC_FLAG_PROCESS_BLOCK,
	};
	return sys_ipc_send(&msg);
}

result_t sys_unblock_process(port_t dest) {
	const ipc_message_t msg = {
		.dest = dest,
		.flags = IPC_FLAG_PROCESS_UNBLOCK,
	};
	return sys_ipc_send(&msg);
}

// ------------- Interrupt Management ----------------
result_t sys_fault_handler(port_t dest) {
	const ipc_message_t msg = {
		.dest = dest,
		.flags = IPC_FLAG_FAULT_HANDLER,
	};
	return sys_ipc_send(&msg);
}

result_t sys_int_register(port_t dest, int8_t interrupt) {
	const ipc_message_t msg = {
		.dest = dest,
		.flags = IPC_FLAG_INT_REGISTER,
	};
	return sys_ipc_send(&msg);
}

result_t sys_int_ack(void) {
	const ipc_message_t msg = {
		.flags = IPC_FLAG_INT_ACK,
	};
	return sys_ipc_send(&msg);
}

result_t sys_port_io(int16_t portio) {
	const ipc_message_t msg = {
		.flags = IPC_FLAG_PORT_IO,
		.value0 = portio
	};
	return sys_ipc_send(&msg);
}

result_t sys_syscall_register(port_t manager) {
	const ipc_message_t msg = {
		.dest = manager,
		.flags = IPC_FLAG_SYSCALL_REGISTER,
	};
	return sys_ipc_send(&msg);
}

// ---------------- Port Management -----------------
port_t sys_create_port(void) {
	ipc_message_t msg = {
		.flags = IPC_FLAG_CREATE_PORT,
	};
	size_t ret = sys_ipc_recv(&msg);
	if(ret != 0) return ret;
	return msg.value0;
}

result_t sys_destroy_port(port_t port) {
	const ipc_message_t msg = {
		.dest = port,
		.flags = IPC_FLAG_DESTROY_PORT,
	};
	return sys_ipc_send(&msg);
}

result_t sys_grant_rights(port_t port, value_t rights_mask) {
	const ipc_message_t msg = {
		.dest = port,
		.value0 = rights_mask,
		.flags = IPC_FLAG_GRANT_RIGHTS,
	};
	return sys_ipc_send(&msg);
}

result_t sys_revoke_rights(port_t port, value_t rights_mask) {
	const ipc_message_t msg = {
		.dest = port,
		.value0 = rights_mask,
		.flags = IPC_FLAG_REVOKE_RIGHTS,
	};
	return sys_ipc_send(&msg);
}

result_t sys_transfer_port(port_t port, port_t dest) {
	const ipc_message_t msg = {
		.source = port,
		.dest = dest,
		.flags = IPC_FLAG_TRANSFER_PORT,
	};
	return sys_ipc_send(&msg);
}

result_t sys_receive_port(port_t port, port_t* recv) {
	ipc_message_t msg = {
		.dest = port,
		.flags = IPC_FLAG_TRANSFER_PORT,
	};
	const size_t ret = sys_ipc_recv(&msg);
	if(ret != 0) return ret;
	return msg.source;
}
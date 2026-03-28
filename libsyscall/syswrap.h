#ifndef __SYSWRAP_BASTION_H
#define __SYSWRAP_BASTION_H
/*
	Medusa system call wrappers so that it's less error prone.
	Each and every wrapper here is a different IPC flag as defined on ipc.h.
*/
#ifdef __cplusplus
extern "C" {
#endif
#include "ipc.h"
typedef int64_t ipc_transfer_t;
typedef int64_t map_result_t;
typedef int64_t result_t ;
typedef int16_t port_t;
typedef size_t virtaddr_t;

// Page management
ipc_transfer_t sys_share_page(port_t dest, virtaddr_t page, size_t n, virtaddr_t wheretomap, size_t mapping_options);
ipc_transfer_t sys_transfer_page(port_t dest, virtaddr_t page, size_t n, virtaddr_t wheretomap, size_t mapping_options);
ipc_transfer_t sys_revoke_page(port_t dest, virtaddr_t page, size_t n, virtaddr_t wheretomap, size_t mapping_options);
map_result_t sys_map_page(virtaddr_t page, size_t n, size_t mapping_options);

// Process management
port_t sys_create_process(void);
result_t sys_kill_process(port_t port);
result_t sys_create_address_space(port_t port);
result_t sys_bind_address_space(port_t port);
result_t sys_wait_process(port_t port);
result_t sys_unblock_process(port_t port);
result_t sys_block_process(port_t port);

// Interrupt management
result_t sys_fault_handler(port_t port);
result_t sys_int_register(port_t dest, int8_t interrupt);
result_t sys_int_ack(void);
result_t sys_port_io(int16_t ioport);
result_t sys_syscall_register(port_t syscall_manager);

// Port management
port_t sys_create_port(void);
result_t sys_destroy_port(port_t port);
result_t sys_grant_rights(port_t port, value_t rights_mask);
result_t sys_revoke_rights(port_t port, value_t rights_mask);
result_t sys_transfer_port(port_t port, port_t destination);
result_t sys_receive_port(port_t port, port_t* revc);

#ifdef __cplusplus
}
#endif

#endif
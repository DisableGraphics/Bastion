#ifndef __IPC_BASTION_H
#define __IPC_BASTION_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stddef.h>
// Error definitions
/// No error
#define EOK 0
/// No destination
#define ENODEST 1
/// Invalid operation
#define EINVALOP 2
/// Invalid message
#define EINVALMSG 3
/// No permissions
#define ENOPERM 4
/// Port has been closed by the owner
#define ENOOWN 5

// Flags
/// Paging flags
#define IPC_FLAG_NONE 0
//// Share a page with destination. Destination and source can both access the page.
#define IPC_FLAG_SHARE_PAGE (1 << 0)
//// Transfer page to destination. Destination owns the page from now on and the source cannot access it in any way.
#define IPC_FLAG_GRANT_PAGE (1 << 1)
//// Take page from destination. Destination will not be allowed to use that page anymore. Effectively source steals from destination.
#define IPC_FLAG_REVOKE_PAGE (1 << 2)

/// Process management flags. All operations can only be used on sys_ipc_send() unless otherwise noted.
//// Creates a new process and returns the port of the process into source.
//// The new process will share the same address space as the process that created it, so it is effectively a thread of the parent.
#define IPC_FLAG_CREATE_PROCESS (1 << 4)
//// Destroys the process behind the dest port. Source process has to have permissions to kill the destination process.
//// Also note that this message is mandatory when finishing a process.
#define IPC_FLAG_KILL_PROCESS (1 << 5)
//// Creates a new address space for the port owner process.
#define IPC_FLAG_NEW_ADDR_SPACE_PROC (1 << 6)
//// Waits for the port owner to close the port. As ports are freed when a process is closed, this can also work as a substitute of wait() in POSIX systems.
//// Can only be used in sys_ipc_recv()
#define IPC_FLAG_WAIT_PROCESS (1 << 7)

/// Synchronisation flags
//// Make the IPC call non blocking (by default IPC calls are blocking). Returns ENODEST if there is no receiver or sender.
#define IPC_FLAG_NONBLOCKING (1 << 8)

/// Interrupt management flags. Unless noted, any flags must only be used on sys_ipc_send()
//// Register to an interrupt. Interrupt number must be added in field value0 of the ipc message.
#define IPC_FLAG_INT_REGISTER (1 << 12)
//// Acknowledge interrupt. If interrupt is not acknowledged, the interrupt source will not emit more interrupts until it is acknowledged. 
#define IPC_FLAG_INT_ACK (1 << 13)
//// Request I/O port acccess. The number of the requested IO port goes into value0. This is only valid for CPU architectures that have I/O ports like x86.
#define IPC_FLAG_PORT_IO (1 << 14)
//// Register port as syscall handler. 
//// When an unknown syscall is emitted and the process has a syscall handler, instead of returning immediately from kernel, a message will be sent to the
//// registered syscall handler to process. Until it has been fully processed, the source process of the syscall will remain blocked.
#define IPC_FLAG_SYSCALL_REGISTER (1 << 15)

/// Port management flags. (If the first two are active at the same time the IPC call will return EINVALOP)
/// Also these options are only to be used with sys_ipc_send by the port owner. If they aren't the resulting operation will return EINVALOP
//// Grant rights to all port holders that aren't the port owner.
#define IPC_FLAG_GRANT_RIGHTS (1 << 16)
//// Revoke rights from all port holders that aren't the port owner.
#define IPC_FLAG_REVOKE_RIGHTS (1 << 17)
//// Copies port descriptor and sends it to a destination port.
#define IPC_FLAG_TRANSFER_PORT (1 << 18)

// Rights
// Rights are granted/revoked using the "value0" field of the message.
// On creation of a port, only the send right is active by default.
/// Allows other processes to send messages via the port (active by default)
#define IPC_RIGHT_SEND (1 << 0)
/// Allows other processes to read from the port.
#define IPC_RIGHT_RECV (1 << 1)
/// Allows other processes to transfer a port (copying of a port).
#define IPC_RIGHT_TRANSF (1 << 2)
/// Allows other processes to kill the owner.
#define IPC_RIGHT_KILL (1 << 3)
/// Allows other processes to grant and revoke port rights.
#define IPC_RIGHT_MODIF_RIGHTS (1 << 4)
/// Allows other processes to grant, revoke and share pages.
#define IPC_RIGHT_PAGING (1 << 5)

// Some type definitions for messages
typedef int16_t port_t;
typedef uint32_t flags_t;
typedef uintptr_t value_t;
typedef uintptr_t page_addr_t;

typedef struct {
	/* Port from which the message is sent */
	port_t source;
	/* Port to which the message is sent */
	port_t dest;
	
	/* Flags for the transfer (Defined in this file in the flags section) */
	flags_t flags;
	/* Values to be sent via this message (Each up to pointer width on the machine) */
	value_t value0;
	value_t value1;
	value_t value2;
	value_t value3;
	value_t value4;

	/* Virtual address of the first page to send */
	page_addr_t page;
	/* Number of pages to send starting from the virtual address defined in the attribute page */
	size_t npages;
} ipc_message_t;

/**
	\brief Send a message via ipc to a port.
	\param msg Message to send.
	\details Sends a message synchronously via IPC to another port defined in the message.
*/
int sys_ipc_send(const ipc_message_t *msg);
/**
	\brief Receive a message via ipc from a port.
	\param msg Message to send.
	\details Receives a message synchronously via IPC.
*/
int sys_ipc_recv(ipc_message_t *msg);

#ifdef __cplusplus
}
#endif

#endif
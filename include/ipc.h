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
#define ENOOWN

// Flags
/// Paging flags
#define IPC_FLAG_NONE 0
//// Share a page with destination
#define IPC_FLAG_SHARE_PAGE (1 << 0)
//// Transfer page to destination
#define IPC_FLAG_GRANT_PAGE (1 << 1)
/// Synchronisation flags
//// Make the IPC call non blocking (by default IPC calls are blocking). Returns ENODEST if there is no receiver or sender.
#define IPC_FLAG_NONBLOCKING (1 << 8)
/// Port management flags. (If the first two are active at the same time the IPC call will return EINVALOP)
/// Also these options are only to be used with sys_ipc_send by the port owner. If they aren't the resulting operation will return EINVALOP
//// Grant rights to all port holders that aren't the port owner.
#define IPC_FLAG_GRANT_RIGHTS (1 << 16)
//// Revoke rights from all port holders that aren't the port owner.
#define IPC_FLAG_REVOKE_RIGHTS (1 << 17)
//// Copies port descriptor and sends it to a destination port.
#define IPC_FLAG_TRANSFER_PORT (1 << 18)

// Rights
// Rights are granted/revoked in the "value" field of the message.
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
	/* Small value to be sent via this message (Up to pointer width on the machine) */
	value_t value;

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
#include "ipc.h"
#include "syscall.h"
#include <limits.h>

#define IPC_SYSCALL_SEND UINT64_MAX
#define IPC_SYSCALL_RECV (UINT64_MAX - 1)

int sys_ipc_send(const ipc_message_t *msg) {
	return syscall((size_t)msg, 0, 0, 0, 0, 0, IPC_SYSCALL_SEND);
}

int sys_ipc_recv(ipc_message_t *msg) {
	return syscall((size_t)msg, 0, 0, 0, 0, 0, IPC_SYSCALL_RECV);
}
# Pseudosyscalls
A pseudosyscall is a syscall in everything but name and invocation. In fact, Medusa only supports 2 syscalls:
- `ipc_send`
- `ipc_recv`

Using the `flags` field in the message the behaviour of these syscalls can be modified.

There are 24 pseudosyscalls (if `ipc_send` and `ipc_recv` are not counted as such).
These are:
| Operation | Description |
|:-| :- |
| IPC_FLAG_SHARE_PAGE | Shares a page into another address space |
| IPC_FLAG_GRANT_PAGE | Transfers a page into another address space |
| IPC_FLAG_REVOKE_PAGE | Removes a page from another address space |
| IPC_FLAG_MAP_PAGE | Changes mapping options of a page |
| IPC_FLAG_CREATE_PROCESS | Creates a process |
| IPC_FLAG_KILL_PROCESS | Kills a process |
| IPC_FLAG_NEW_ADDR_SPACE_PROC | Creates a new address space |
| IPC_FLAG_BIND_ADDR_SPACE | Binds a process with an address space |
| IPC_FLAG_START_PROCESS | Starts a process |
| IPC_FLAG_WAIT_PROCESS | Waits for a process to end |
| IPC_FLAG_PROCESS_SLEEP | Makes a process sleep for n millis |
| IPC_FLAG_PROCESS_BLOCK | Blocks a process |
| IPC_FLAG_PROCESS_UNBLOCK | Unblocks a process |
| IPC_FLAG_NONBLOCKING | Tries to send a message. If the destination is not listening returns immediately |
| IPC_FLAG_FAULT_HANDLER | Binds another process as a fault handler |
| IPC_FLAG_INT_REGISTER | Registers a process for an interrupt |
| IPC_FLAG_INT_ACK | Acknowledges the interrupt |
| IPC_FLAG_PORT_IO | (x86-64 only) Asks the kernel to allow port IO |
| IPC_FLAG_SYSCALL_REGISTER | Registers a process as a system call handler |
| IPC_FLAG_GRANT_RIGHTS | Grants rights to a port |
| IPC_FLAG_REVOKE_RIGHTS | Revokes rights from a port |
| IPC_FLAG_TRANSFER_PORT | Shares a port with another process |
| IPC_FLAG_CREATE_PORT | Creates a port |
| IPC_FLAG_DESTROY_PORT | Destroys a port |

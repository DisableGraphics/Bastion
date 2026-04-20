# Process
A process is an execution context (CPU registers) with:
- An address space (which can be shared).
- A series of ports (unless transferred, local to the process).

## Process VS Process
A process in the medusa microkernel ecosystem is a thread in UNIX parlance. The concept of processes (as in UNIX) doesn't exist for the medusa microkernel: processes (as in UNIX) are just groups of processes (as in medusa) and as such handled by a user space manager.

## Ports in a process
A process always has at least one port: the bootstrap port. It cannot be deleted, it cannot have its permissions changed and the whole idea of this port is to provide user space managers with a mechanism to kill or modify a process if something goes wrong.
Ports are local to the process, so port numbers in one process do not translate to other processes. 
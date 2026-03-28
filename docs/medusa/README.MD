# Medusa
Medusa is the name that the microkernel receives. It has support for the following functionality:
- Preemptive multitasking using a MFQ algorithm.
- Synchronous IPC
	- System calls built on the IPC subsystem.
	- Transferring unknown system calls to syscall handlers
- Memory management
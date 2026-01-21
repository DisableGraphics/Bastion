# Bastion
Operating system built on a microkernel.
Microkernel supports:
- Synchronous buffer-less IPC based on Ports.
- MFQ scheduler.
- PIC.
- Local APIC timer.
- Up to AVX2 & SSE (I don't have any AVX512 machines to test, but if I get my hands on one of those you bet I will be adding support)
- System call delegation to user-space syscall servers.
- Virtual Memory.
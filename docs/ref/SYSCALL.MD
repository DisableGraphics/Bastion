# SYSCALL for AMD64
It's really confusing so I'm writing everything needed to use the `syscall` instruction in order to trap into the kernel.

## General info
There are 3 registers (MSRs) that are used on the setup and operation of the `syscall` instruction.
These are:
- STAR (0xC0000081)
	- Contains userland CS & SS to be loaded into those registers when executing `sysret`
		- More concretely: CS is loaded from bits 63:48 and will have that value plus 16
		- SS ditto but instead of + 16 it would be + 8
	- To be more concrete:
		- if STAR contains the value 0x0003_0000_0000_0000 -> then CS would be 0x0003 + 16 = 0x13 and SS would be 0x0003 + 8 = 0x0B
- LSTAR (0xC0000082)
	- Contains the address of the first instruction of the `syscall` handler.
		- %rip will be set to that value on syscall.
- SFMASK (0xC0000084)
	- It contains a bitmask which will be applied to RFLAGS on syscall. Each bit set to one on this register will be set to 0 on RFLAGS.
- EFER (0xC0000080)
	- Bit #0 contains a bit which enables the `syscall` instruction if set to 1.

## Traps
- RSP is not loaded to any value. This is the responsibility of the syscall handler. (There are tricks for this like using swapgs)
- Beware of data races for NMIs and MCEs interrupts -> they cannot be ignored. It's just easier to set up another kernel stack for them and behave accordingly. 

## Setting up
Setting the `syscall` instruction can be done by following these steps:
0) (Before setting up anything): Finding a way to get the kernel stack %rsp so that the syscall handler does not leak data into user space.
- Usually the pattern is using either registers %gs or %fs in order to get the RSP. Following is a list of possible solutions:
	- The instruction `swapgs` is a good solution and has the following design implementations:
		- On task scheduling save the kernel stack address into MSR C0000102 (IA32_KERNEL_GS_BASE) and put some rubbish value into %gs (like 0)
			- On the syscall handler, save %rsp into %gs and do a `swapgs`, then do another `swapgs` at the end of the syscall handler (again to not leak info to user space).
		- Alternatively instead of the RSP you can keep a table of per-processor data and on task switch just update the table instead of gs.
	- Instead of using swapgs you can try to use %fs, but there is not a dedicated instruction so you'd have to do the swap manually.
		- You can reuse IA32_KERNEL_GS_BASE but since there is no `swapgs` instruction you'd need to use the `wrmsr` and `rdmsr` instructions.
1) Setting bit 0x200 on SFMASK.
	- That bit is the interrupt flag, which means that interrupts are disabled while the syscall handler runs.
	- If your kernel is fully interruptible then you don't need this step, but usually kernels aren't fully interruptible so this step is highly recommended to follow.
2) 

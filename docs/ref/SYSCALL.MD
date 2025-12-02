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
- %rsp is not loaded to any value. This is the responsibility of the syscall handler. (There are tricks for this like using swapgs)
- Beware of data races for NMIs and MCEs interrupts as they cannot be ignored, even with cli. A simple solution is to set up another kernel stack for them and set their TSS interrupt stack field when you set up interrupts and/or the TSS.
- %rcx & %r11 must be preserved in some way. Concretely, the System V ABI stablishes %rcx as a "scratch" register, while %r11 is a callee-saved one. This means that usually you should be more aware of %rcx.  

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
	- Also try to write a syscall handler. It can be just a simple system call handler, like printing "hello" on the screen.

1) Setting bit 0x200 on SFMASK (and optionally other flags).
	- That bit is the interrupt flag, which means that interrupts are disabled while the syscall handler runs.
	- If your kernel is fully interruptible then you don't need this step, but usually kernels aren't, so this step is highly recommended to follow.
	```c
	#define INTERRUPT_FLAG 0x200
	wrmsr(IA32_SFMASK, rdmsr(IA32_SFMASK) | INTERRUPT_FLAG);
	```
2) Set values of CS & SS in the STAR register.
	- The GDT should have these two entries in the following order:
		- User Data
		- User Code
	- Then just save in the STAR register `(offset of User Data in the GDT - 8) << 48` 
	```c
	wrmsr(IA32_STAR, (int64_t)(user_data_offset_gdt - 8) << 48)
	```
3) Save address of syscall handler into the LSTAR MSR.
	- Just grab a pointer to the function and stuff it into LSTAR with `wrmsr`.
	```c
	wrmsr(IA32_LSTAR, (int64_t)(&syscall_handler));
	```
4) Enable the `syscall` instruction
	- Load EFER, enable bit #0 and save the result into EFER
	```c
	#define ENABLE_SYSCALL 1
	wrmsr(IA32_EFER, rdmsr(IA32_EFER) | ENABLE_SYSCALL);
	```
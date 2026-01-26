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

# Building
The build process has been designed to be as flexible as possible.
To configure the OS build it is necessary to have rust installed, because the configuration system is written in rust.
It can be run just once or it can be run multiple times to change modules and components of the operating system.
To configure the build, just run
```bash
./cfgmodules.sh
```

> [!WARNING]
> Heads up: unless you know what you are doing, it is recommended to use the default configuration for the cfgmodules system.

After selecting all necessary options (or just pressing Escape to end up with the recommended default configuration for x86_64 systems), you can proceed to compile the OS:

```bash
./compile.sh
./diskimg.sh
# Optionally, to run or test in a VM
# ./qemu.sh
```
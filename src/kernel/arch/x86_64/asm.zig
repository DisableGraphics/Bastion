pub fn outb(port: u16, value: u8) void {
	asm volatile (
		\\outb %[value], %[port]
		:
		: [port] "N{dx}" (port), [value] "{al}" (value)
	);
}

pub fn inb(port: u16) u8 {
	return asm volatile (
		\\inb %[port], %[ret]
		: [ret] "={al}" (-> u8),
		: [port] "N{dx}" (port),
	);
}

pub fn read_cr4() u64 {
	return asm volatile (
		\\mov %%cr4, %[ret]
		: [ret] "=r" (-> u64),
		:
	);
}

pub fn read_cr3() u64 {
	return asm volatile (
		\\mov %%cr3, %[ret]
		: [ret] "=r" (-> u64),
		:
	);
}

pub fn io_wait() void {
    outb(0x80, 0);
}

pub fn read_msr(msr: u64) u64 {
	var low: u32 = undefined;
	var high: u32 = undefined;
	asm volatile (
        "rdmsr"
        : [low]"={eax}"(low), [high]"={edx}"(high)
        : [inp]"{ecx}"(msr)
    );
	return (@as(u64, high) << 32) | low;
}

pub fn write_msr(msr: u64, value: u64) void {
	const low: u32 = @as(u32, @truncate(value));
	const high: u32 = @truncate(value >> 32);
	asm volatile (
        "wrmsr"
        :
        : [msr]"{ecx}"(msr), [low]"{eax}"(low), [high]"{edx}"(high)
    );
}

pub fn mfence() void {
	asm volatile("mfence");
}

pub inline fn irqdisable() u64 {
	return 
	asm volatile ("pushf\n\tcli\n\tpop %[flags]" : [flags]"=r"(->u64) : : "memory");
}

pub inline fn irqrestore(mask: u64) void {
	asm volatile ("push %[mask]\n\tpopf" : : [mask]"rm"(mask) : "memory","cc");
}

pub inline fn rdtsc() u64 {
	var low: u32 = undefined; var high: u32 = undefined;
    asm volatile("rdtsc":[low]"={eax}"(low),[high]"={edx}"(high));
    return (@as(u64, high) << 32) | low;
}
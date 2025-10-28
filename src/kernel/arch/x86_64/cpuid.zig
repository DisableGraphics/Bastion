pub fn cpuid(eax: u32, ecx: u32) struct { eax: u32, ebx: u32, ecx: u32, edx: u32 } {
	var a: u32 = eax;
	var b: u32 = 0;
	var c: u32 = ecx;
	var d: u32 = 0;

	asm volatile ("cpuid"
		: [a] "={eax}" (a),
		  [b] "={ebx}" (b),
		  [c] "={ecx}" (c),
		  [d] "={edx}" (d)
		: [in_a] "{eax}" (eax),
		  [in_c] "{ecx}" (ecx)
	);

	return .{ .eax = a, .ebx = b, .ecx = c, .edx = d };
}

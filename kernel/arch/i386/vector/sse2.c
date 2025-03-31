#include <kernel/vector/sse2.h>
#include <cpuid.h>


int sse2_available() {
	int edx, unused;
	__cpuid(1, unused, unused, unused, edx);
	return edx & (1 << 26);
}
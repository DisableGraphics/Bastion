#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

struct regs {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip, cs, eflags, ss;
};

#define COPY_REGS(regs) do {\
	__asm__ volatile ( \
        "mov %%eax, %0\n\t" \
        "mov %%ebx, %1\n\t" \
        "mov %%ecx, %2\n\t" \
        "mov %%edx, %3\n\t" \
        "mov %%esi, %4\n\t" \
        "mov %%edi, %5\n\t" \
        "mov %%ebp, %6\n\t" \
        "mov %%esp, %7\n\t" \
        : "=m"(r.eax), "=m"(r.ebx), "=m"(r.ecx), "=m"(r.edx), \
          "=m"(r.esi), "=m"(r.edi), "=m"(r.ebp), "=m"(r.esp) \
    ); \
	} while(0)

#ifdef __cplusplus
}
#endif
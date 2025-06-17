#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void print_disassemble(void *addr, size_t n, void* current_eip);

#ifdef __cplusplus
}
#endif
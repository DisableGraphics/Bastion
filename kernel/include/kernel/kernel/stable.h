#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t address;
    const char* name;
} symbol_t;

extern symbol_t kernel_symbols[];
extern const size_t kernel_symbol_count;
const char* find_function_name(size_t ip);

#ifdef __cplusplus
}
#endif

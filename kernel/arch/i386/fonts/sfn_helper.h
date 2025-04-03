#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stddef.h>
void sfn_init(uint8_t* backbuffer, size_t pitch);
void sfn_putc(unsigned unicode);
#ifdef  __cplusplus
}
#endif
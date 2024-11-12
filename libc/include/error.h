#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>

#define kerror(fmt, ...) printf(fmt, __VA_ARGS__); __asm__ __volatile__("hlt");

#ifdef __cplusplus
}
#endif
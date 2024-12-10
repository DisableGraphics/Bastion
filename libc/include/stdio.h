#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>

#define EOF (-1)

#ifdef __cplusplus
extern "C" {
#endif

int printf(const char* __restrict, ...);
int serial_printf(const char* __restrict, ...);
int putchar(int);
int putchar_serial(int);
int puts(const char*);

#ifdef __cplusplus
}
#endif

#endif

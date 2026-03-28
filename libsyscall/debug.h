#ifndef __BASTION_DEBUG
#define __BASTION_DEBUG

#ifdef __cplusplus
extern "C" {
#endif
#include "ipc.h"

int debug_init(port_t port);
int debug_write(const char* __restrict fmt, ...);
void puts(char* c);
void putc(char c);

#ifdef __cplusplus
}
#endif
#endif
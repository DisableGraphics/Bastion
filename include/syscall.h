#ifndef __SYSCALL_BASTION_H
#define __SYSCALL_BASTION_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

size_t syscall(size_t callno, size_t arg0, size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5);

#ifdef __cplusplus
}
#endif
#endif
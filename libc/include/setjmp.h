#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void *jmp_buf[6];

__attribute__((naked,returns_twice))
int setjmp(jmp_buf buf);
__attribute__((naked,noreturn))
void longjmp(jmp_buf buf, int ret);

#ifdef __cplusplus
}
#endif
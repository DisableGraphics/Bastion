#include <stdint.h>
#include <kernel/kernel/panic.hpp>
#if UINT32_MAX == UINTPTR_MAX
#define STACK_CHK_GUARD 0xe2dee396
#else
#define STACK_CHK_GUARD 0x595e9fbd94fda766
#endif

extern uintptr_t __stack_chk_guard;
extern "C" [[gnu::noreturn]] void __stack_chk_fail(void);
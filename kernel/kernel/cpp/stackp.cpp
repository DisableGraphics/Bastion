#include <kernel/cpp/stackp.hpp>

uintptr_t __stack_chk_guard = STACK_CHK_GUARD;
[[gnu::noreturn]]
void __stack_chk_fail(void)
{
	kn::panic("STACK SMASHING DETECTED");
}
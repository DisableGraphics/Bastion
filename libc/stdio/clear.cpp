#include <stdio.h>
#include <kernel/drivers/tty/ttyman.hpp>

void clear() {
	TTYManager::get().clear();
}
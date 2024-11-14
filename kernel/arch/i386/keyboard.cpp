#include <kernel/keyboard.hpp>

Keyboard& Keyboard::get() {
	static Keyboard instance;
	return instance;
}
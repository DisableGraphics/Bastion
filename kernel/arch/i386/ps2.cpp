#include <kernel/ps2.hpp>

PS2Controller &PS2Controller::get() {
	static PS2Controller instance;
	return instance;
}

void PS2Controller::init() {
	
}
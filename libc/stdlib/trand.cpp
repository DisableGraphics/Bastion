#include <kernel/random/rand.hpp>

extern "C" int trand(void) {
	return ktrand();
}
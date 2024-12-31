#include <stdlib.h>
#include <stdint.h>

// The following functions define a portable implementation of rand and srand.

static uint32_t next = 1;

extern "C" int rand(void) {  // RAND_MAX assumed to be 32767
    next = next * 1103515245 + 12345;
    return (unsigned int) (next / 65536) % RAND_MAX;
}

extern "C" void srand(unsigned int seed) {
    next = seed;
}
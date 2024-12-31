#include <stdlib.h>
#include <stdint.h>

// The following functions define a portable implementation of rand and srand.

static uint64_t rand_state = 123456789;

#define RAND_A 6364136223846793005ULL
#define RAND_C 1ULL

extern "C" int rand() {
    rand_state = rand_state * RAND_A + RAND_C;
    return (int)(rand_state >> 32); // Use higher-order bits for better randomness
}

extern "C" void srand(unsigned int seed) {
    rand_state = seed;
}
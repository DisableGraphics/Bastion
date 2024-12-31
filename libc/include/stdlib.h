#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <sys/cdefs.h>
#include <liballoc/liballoc.h>

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((__noreturn__))
void abort(void);

#define RAND_MAX 32768

int rand(void);
void srand(unsigned int seed);

// True random number generator (Platform-dependent)
// Returns -1 if a number can't be generated 
// (i.e lack of HW support)
int trand(void);

#ifdef __cplusplus
}
#endif

#endif

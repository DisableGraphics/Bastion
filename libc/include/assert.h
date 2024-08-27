#pragma once
#include "error.h"
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
#define assert(expr) do { if(!(expr)) { kerror("Error at line %d in file %s: Assertion \"" #expr "\" failed", __LINE__, __FILE__) } } while(0);
#ifdef __cplusplus
}
#endif
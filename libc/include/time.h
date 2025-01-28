#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef int64_t time_t;

time_t time(time_t* tloc);
time_t seconds_since_boot();

#ifdef __cplusplus
}
#endif
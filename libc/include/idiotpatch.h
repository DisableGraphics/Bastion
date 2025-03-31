#pragma once
#include <liballoc/liballoc.h>

#ifdef __cplusplus
extern "C" {
#endif

static void *(*malloc)(size_t) = kmalloc;
static void *(*realloc)(void*, size_t) = krealloc;
static void *(*calloc)(size_t, size_t) = kcalloc;
static void (*free)(void*) = kfree;

#ifdef __cplusplus
}
#endif
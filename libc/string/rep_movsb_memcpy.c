#include <stddef.h>
#include <string.h>

void rep_movsb_memcpy(void *dest, const void *src, size_t n) {
    __asm__ __volatile__("rep movsb"
                 : "+D"(dest), "+S"(src), "+c"(n)
                 :
                 : "memory");
}
#include <stdlib.h>
#define malloc(size) kmalloc(size)
#define free(ptr) kfree(ptr)
#include <emmintrin.h>
#include <string.h>
#include <kernel/vector/sse2.h>

void *sse2_memset(void *dest, int c, size_t n) {
    if (!dest || !n) return dest;
    
    // Fallback to regular memset if SSE2 not available
    if (!sse2_available()) {
        return memset(dest, c, n);
    }

    unsigned char *d = dest;
    unsigned char value = (unsigned char)c;
    
    // Handle misaligned start (0-15 bytes)
    while (((uintptr_t)d & 0xF) && n) {
        *d++ = value;
        n--;
    }

    if (n >= 16) {
        // Create 128-bit pattern of the byte value
        __m128i pattern = _mm_set1_epi8(value);
        
        // Aligned stores for main block
        size_t blocks = (n>>4);
        n &= 0xf;
        
        while (blocks--) {
            _mm_store_si128((__m128i*)d, pattern);
            d += 16;
        }
    }

    // Handle remaining bytes (0-15)
    while (n--) {
        *d++ = value;
    }

    return dest;
}
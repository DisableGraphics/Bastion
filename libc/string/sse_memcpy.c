#include <idiotpatch.h>
#include <emmintrin.h>
#include <string.h>
#include <kernel/vector/sse2.h>

void sse2_memcpy(void *dest, const void *src, size_t n) {
	if (!dest || !src) return;

	if(!sse2_available()) memcpy(dest, src, n);

	if (n == 0) return;

	uintptr_t align_offset = (uintptr_t)dest % 16;
    if (align_offset != 0) {
        size_t bytes_to_align = 16 - align_offset;
        if (bytes_to_align > n) bytes_to_align = n;
        
        uint8_t *d = (uint8_t *)dest;
        const uint8_t *s = (const uint8_t *)src;
        for (size_t i = 0; i < bytes_to_align; i++) {
            *d++ = *s++;
        }
        dest = d;
        src = s;
        n -= bytes_to_align;
    }

    __m128i *d = (__m128i *)dest;
    const __m128i *s = (const __m128i *)src;

    for(; n >= 16; n -= 16) {
        _mm_storeu_si128(d++, _mm_loadu_si128(s++));
		if(!(n & 255)) _mm_mfence();
    }

    // Handle remaining bytes
    uint8_t *d_byte = (uint8_t *)d;
    const uint8_t *s_byte = (const uint8_t *)s;
    while(n--) {
        *d_byte++ = *s_byte++;
    }
	_mm_mfence();
}
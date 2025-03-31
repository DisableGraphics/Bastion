#include <kernel/drivers/vesa/vesahelp.h>
#include <emmintrin.h>

void fast_clear(uint32_t color, uint8_t* fb, size_t pixel_count) {
    __m128i fill = _mm_set1_epi32(color);
    __m128i* ptr = (__m128i*)fb;
    const size_t endsize = pixel_count >> 4;
    for (size_t i = 0; i < endsize; i++) {
        _mm_stream_si128(ptr + i, fill);
    }
    _mm_sfence();
}
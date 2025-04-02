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

void draw_rectangle(int x1, int y1, int x2, int y2, uint32_t packed_color, uint8_t* backbuffer, size_t* row_pointers, uint32_t depth_disp) {  
	__m128i color_vec;
	color_vec = _mm_set1_epi32(packed_color);
	size_t bytes_pp = sizeof(uint32_t);
	
	for (int y = y1; y <= y2; y++) {
		uint8_t* ptr = backbuffer + row_pointers[y] + (x1 << depth_disp);
		int pixels_left = x2 - x1 + 1;
		
		// Align to 16-byte boundary
		while (((uintptr_t)ptr & 0xf) != 0 && pixels_left > 0) {
			*(uint32_t*)(ptr) = packed_color;
			ptr += bytes_pp;
			pixels_left--;
		}
		
		// Process 4 pixels at a time (16 bytes for 32bpp)
		int simd_pixels = pixels_left >> 2;
		for (int i = 0; i < simd_pixels; i++) {
			_mm_store_si128((__m128i*)(ptr), color_vec);
			ptr += bytes_pp << 2;
		}
		
		// Remaining pixels
		pixels_left &= 0x3;
		while (pixels_left-- > 0) {
			*(uint32_t*)(ptr) = packed_color;
			ptr += bytes_pp;
		}
	}
}

void draw_pixels(int x1, int y1, int width, int height, 
                     uint8_t* pixels, uint8_t* backbuffer, 
                     size_t* row_pointers, uint32_t depth_disp, uint32_t pitch) {
    // Input validation
    if (width < 1 || height < 1 || !pixels || !backbuffer || !row_pointers) return;
    
    const int bytes_per_pixel = 4; // Assuming 32bpp
    const int src_stride = (width << depth_disp);
    
    // Process each row
    for (int y = y1; y <= (height + y1); y++) {
        const uint8_t* src_row = pixels + (y - y1) * src_stride;
        uint8_t* dst_row = backbuffer + row_pointers[y] + (x1 << depth_disp);
        
        // Calculate alignment offset
        size_t misalign = (uintptr_t)dst_row & 0xF;
        int aligned_x = (misalign ? ((16 - misalign) >> depth_disp) : 0);
        
        // Copy unaligned beginning pixels
        for (int x = 0; x < aligned_x && x < width; x++) {
            *(uint32_t*)(dst_row + x * bytes_per_pixel) = 
                *(const uint32_t*)(src_row + x * bytes_per_pixel);
        }
        
        // SSE2-optimized main copy (16 bytes = 4 pixels at a time)
        const int sse_pixels = (width - aligned_x) >> 2;
        const __m128i* sse_src = (const __m128i*)(src_row + aligned_x * bytes_per_pixel);
        __m128i* sse_dst = (__m128i*)(dst_row + aligned_x * bytes_per_pixel);
        
        for (int i = 0; i < sse_pixels; i++) {
            _mm_storeu_si128(sse_dst + i, _mm_loadu_si128(sse_src + i));
        }
        
        // Handle remaining pixels
        const int processed = aligned_x + (sse_pixels << 2);
        for (int x = processed; x < width; x++) {
            *(uint32_t*)(dst_row + x * bytes_per_pixel) = 
                *(const uint32_t*)(src_row + x * bytes_per_pixel);
        }
    }
    
    // Memory fence for write combining
    _mm_sfence();
}
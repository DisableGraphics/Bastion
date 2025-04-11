#include <kernel/drivers/vesa/vesa.hpp>
#include <kernel/drivers/vesa/vesahelp.h>
#include <string.h>
#include <kernel/kernel/log.hpp>
#include <kernel/assembly/inlineasm.h>
#include <kernel/memory/page.hpp>
#include <kernel/memory/mmanager.hpp>
#include <kernel/cpp/min.hpp>
#include <kernel/cpp/max.hpp>
#define SSFN_CONSOLEBITMAP_TRUECOLOR
#include <ssfn/ssfn.h>

#define DRAW_RAW(buffer, c) switch(depth) { \
	case 32: \
		*reinterpret_cast<uint32_t*>(buffer) = (c.r << red_pos) | (c.g << green_pos) | (c.b << blue_pos); \
		break; \
	case 16: \
	case 15: {\
		hal::color squished { \
			squish8_to_size(c.r, red_size), \
			squish8_to_size(c.g, green_size), \
			squish8_to_size(c.b, blue_size), \
		}; \
		uint16_t dest = 0; \
		dest |= squished.r << red_pos; \
		dest |= squished.g << green_pos; \
		dest |= squished.b << blue_pos; \
		*reinterpret_cast<uint16_t*>(where) = dest; \
		break; \
	}\
	case 24: \
	default: \
		*reinterpret_cast<uint32_t*>(buffer) = (c.r << red_pos) | (c.g << green_pos) | (c.b << blue_pos); \
}

VESADriver::VESADriver(uint8_t* framebuffer,
	uint32_t width,
	uint32_t height,
	uint32_t pitch,
	uint32_t depth,
	uint32_t red_pos,
	uint32_t green_pos,
	uint32_t blue_pos,
	uint32_t red_size,
	uint32_t green_size,
	uint32_t blue_size) : 
	hal::VideoDriver(framebuffer,
		width,
		height,
		pitch,
		depth),
		red_pos(red_pos),
		green_pos(green_pos),
		blue_pos(blue_pos),
		red_size(red_size),
		green_size(green_size),
		blue_size(blue_size)
{
	// Map all framebuffer pages.
	// The reason for the WRITE_THROUGH bit is that the PAT region is the region #1,
	// which requires bit 1 for PAT active (WRITE_THROUGH)
	bool is_pat_enabled = PagingManager::get().enable_pat_if_it_exists();
	auto fbsize_pages = (scrsize + PAGE_SIZE - 1)/PAGE_SIZE;
	auto fbsize_regions = (scrsize + REGION_SIZE - 1)/REGION_SIZE;
	if(!PagingManager::get().page_table_exists(reinterpret_cast<void*>(framebuffer))) {
		for(size_t region = 0; region < fbsize_regions; region++) {
			void* newpagetable = MemoryManager::get().alloc_pages(1, CACHE_DISABLE | READ_WRITE);
			void* fbaddroff = reinterpret_cast<void*>(framebuffer + region*REGION_SIZE);
			PagingManager::get().new_page_table(newpagetable, 
				fbaddroff);
			for(auto pages = 0; pages < PAGE_SIZE; pages++) {
				void* fbaddroff_p = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(fbaddroff) + pages*PAGE_SIZE);
				PagingManager::get().map_page(fbaddroff_p, 
					fbaddroff_p, 
					(is_pat_enabled ? (PAT | WRITE_THROUGH) : 0) | READ_WRITE);
			}
		}
	}

	switch(depth) {
		case 32:
			depth_disp = 2;
			break;
		case 24:
			depth_disp = 1;
			break;
		case 16:
		case 15:
			depth_disp = 1;
			break;
		default:
			// Shitty default
			depth_disp = 1;
	}

	log(INFO, R"(VESA Framebuffer with:
	- framebuffer: %p,
	- size of framebuffer: %p,
	- width: %d,
	- height: %d,
	- pitch: %d,
	- depth: %d,
	- red_pos: %d,
	- green_pos: %d,
	- blue_pos: %d,
	- red_mask: %d,
	- green_mask: %d,
	- blue_mask: %d)",
	framebuffer,
	scrsize,
	width,
	height,
	pitch,
	depth,
	red_pos,
	green_pos,
	blue_pos,
	red_size,
	green_size,
	blue_size);
	log(INFO, "Back buffer: %p", backbuffer);
	// Write-combine when PAT is not available using
	// MSRs
	if(!is_pat_enabled) {
		uint64_t mask = ~(scrsize - 1) | 0x800;
		uint64_t base = reinterpret_cast<uint64_t>(framebuffer) | 0x0C;
		wrmsr(MSR_MTRRphysBase0, base);
		wrmsr(MSR_MTRRphysMask0, mask);
	}

	// Precompute row pointers
	for(size_t i = 0; i < height; i++) {
		row_pointers[i] = pitch*i;
	}
}

void VESADriver::init() {
}

void VESADriver::handle_interrupt() {
	// Do nothing. The interrupt is handled by the timer
}

bool VESADriver::is_text_only() {
	return false;
}

void VESADriver::draw_char(unsigned c, int x, int y) {
	ssfn_dst.x = x;
	ssfn_dst.y = y;
	mark_rectangle_as_dirty(x, y, x+ssfn_src->width, y+ssfn_src->height);
	ssfn_putc(c);
}

void VESADriver::draw_pixel(int x, int y, hal::color c) {
	dirty = true;
	unsigned where = (x<<depth_disp) + row_pointers[y];
	mark_tile_as_dirty(x, y);
	DRAW_RAW(backbuffer+where, c);
}

void VESADriver::draw_rectangle(int x1, int y1, int x2, int y2, hal::color c) {
	dirty = true;
	x1 = max<int>(0, min<int>(x1, width-1));
	y1 = max<int>(0, min<int>(y1, height-1));
	x2 = max<int>(0, min<int>(x2, width-1));
	y2 = max<int>(0, min<int>(y2, height-1));
	mark_rectangle_as_dirty(x1, y1, x2, y2);
	uint32_t packed_color = (c.r << red_pos) | (c.g << green_pos) | (c.b << blue_pos);
	::draw_rectangle(x1, y1, x2, y2, packed_color, backbuffer, row_pointers, depth_disp);
}

void VESADriver::draw_pixels(int x1, int y1, int w, int h, uint8_t* data) {
	dirty = true;
	if(y1 + h >= height) {
		h = height - y1;
	}
	mark_rectangle_as_dirty(x1, y1, x1 + w - 1, y1 + h - 1);
	::draw_pixels(x1, y1, w, h, data, backbuffer, row_pointers, depth_disp, w << depth_disp);
}

void VESADriver::mark_rectangle_as_dirty(int x1, int y1, int x2, int y2) {
	int tile_x0 = x1 >> TILE_SIZE_DISP;
    int tile_y0 = y1 >> TILE_SIZE_DISP;
    int tile_x1 = x2 >> TILE_SIZE_DISP;
    int tile_y1 = y2 >> TILE_SIZE_DISP;

    for (int ty = tile_y0; ty <= tile_y1; ty++) {
		const size_t tile_offset = ty * tiles_x;
        for (int tx = tile_x0; tx <= tile_x1; tx++) {
            dirty_tiles[tile_offset + tx] = true;
			dirty_tiles_for_clear[tile_offset + tx] = true;
        }
    }
}

void VESADriver::clear(hal::color c) {
	dirty = true;
	const int color = (c.r << red_pos) | (c.g << green_pos) | (c.b << blue_pos);

	for (size_t y = 0; y < tiles_y; y++) {
		const size_t row_offset = y * tiles_x;
		int y0 = y << TILE_SIZE_DISP;
		for (size_t x = 0; x < tiles_x; x++) {
			const size_t index = row_offset + x;
			if (!dirty_tiles_for_clear[index])
				continue;
			int x0 = x << TILE_SIZE_DISP;
			for (int ty = 0; ty < TILE_SIZE; ty++) {
				uint8_t* row = backbuffer + (y0 + ty) * pitch + (x0 << depth_disp);
				fast_clear(color, row, TILE_SIZE << depth_disp);
			}
			dirty_tiles[index] = true;
			dirty_tiles_for_clear[index] = false;
		}
	}
}

uint8_t VESADriver::squish8_to_size(int val, uint8_t destsize) {
	int value = val * destsize;
	return (value + (value >> 8) + 1) >> 8;
}

void VESADriver::set_fonts(uint8_t* fontsarr) {
	ssfn_src = (ssfn_font_t*)fontsarr;
	ssfn_dst.ptr = backbuffer;
	ssfn_dst.p = pitch;
	ssfn_dst.fg = 0xFFFFFFFF;
	ssfn_dst.bg = 0;
	ssfn_dst.x = 100;
	ssfn_dst.y = 200;
}

uint8_t VESADriver::get_font_width() {
	return ssfn_src->width;
}
uint8_t VESADriver::get_font_height() {
	return ssfn_src->height;
}
uint32_t VESADriver::get_width() {
	return width;
}
uint32_t VESADriver::get_height() {
	return height;
}
uint32_t VESADriver::get_pitch() {
	return pitch;
}
uint32_t VESADriver::get_depth() {
	return depth;
}
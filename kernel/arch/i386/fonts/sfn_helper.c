#define SSFN_CONSOLEBITMAP_TRUECOLOR
#include "sfn_init.h"
#include <ssfn/ssfn.h>

extern unsigned char _binary_console_sfn_start;

void sfn_init(uint8_t* backbuffer, size_t pitch) {
	// ssfn data to be able to write shit
	ssfn_src = (ssfn_font_t*)&_binary_console_sfn_start;
	ssfn_dst.ptr = backbuffer;
	ssfn_dst.p = pitch;
	ssfn_dst.fg = 0xFFFFFFFF;
	ssfn_dst.bg = 0;
	ssfn_dst.x = 0;
	ssfn_dst.y = 0;
}
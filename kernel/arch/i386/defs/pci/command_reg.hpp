#pragma once
#define IO_SPACE 1
#define MEM_SPACE (1 << 1)
#define BUS_MASTER (1 << 2)
#define SPECIAL_CYCLES (1 << 3)
#define MEM_WR_INVALIDATE_ENABLE (1 << 4)
#define VGA_PAL_SNOOP (1 << 5)
#define PAR_ERR_RESP (1 << 6)
#define SERR_ENABLE (1 << 8)
#define FB2B_ENABLE (1 << 9)
#define INT_DISABLE (1 << 10)
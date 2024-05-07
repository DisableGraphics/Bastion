#include "vga.hpp"

uint8_t VGA::entry_color(VGA::vga_color fg, VGA::vga_color bg)
{
    return fg | bg << 4;
}

uint16_t VGA::entry(unsigned char uc, uint8_t color)
{
    return (uint16_t) uc | (uint16_t) color << 8;
}
#pragma once
#include <stdint.h>

#pragma pack(1)
struct gdtr_t
{
    uint16_t limit;
    uint32_t base;
};
#pragma pack()
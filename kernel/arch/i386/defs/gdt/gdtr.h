#pragma once
#include <stdint.h>

struct [[gnu::packed]] gdtr_t
{
    uint16_t limit;
    uint32_t base;
};
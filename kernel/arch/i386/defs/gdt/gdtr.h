#pragma once
#include <stdint.h>
/**
	\brief GDT register to tell the CPU where is the GDT
 */
struct [[gnu::packed]] gdtr_t
{
    uint16_t limit;
    uint32_t base;
};
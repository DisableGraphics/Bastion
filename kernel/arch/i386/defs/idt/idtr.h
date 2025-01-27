#pragma once
#include <stdint.h>
/**
	\brief IDT register to tell the CPU where the IDT is
 */
struct [[gnu::packed]] idtr_t {
	uint16_t	limit;
	uint32_t	base;
};
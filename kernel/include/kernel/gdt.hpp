#pragma once
#include <stdint.h>
#ifdef __i386
#include "../arch/i386/defs/gdt_entry.h"
#include "../arch/i386/defs/gdtr.h"
#include "../arch/i386/defs/gdt_size.h"
#endif

// Global Descriptor Table
class GDT {
	public:
		static GDT &get();
		void init();
		void encodeEntry(uint8_t *target, GDT_entry source);
	private:
		void load_gdtr(gdtr_t gdtr);
		gdtr_t gdtr;
		uint8_t tss[TSS_SIZE];
		uint8_t gdt[GDT_SIZE];
		GDT();
};
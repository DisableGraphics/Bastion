#pragma once
#include <stdint.h>
#ifdef __i386
#include "../arch/i386/defs/gdt_entry.h"
#include "../arch/i386/defs/gdtr.h"
#include "../arch/i386/defs/gdt_size.h"
#endif

/**
	\brief Global Descriptor Table
	Note: this is an x86 only structure.
	Other platforms will do nothing with this
 */
class GDT {
	public:
		/**
			\brief get singleton instance
		 */
		static GDT &get();
		/**
			\brief Initialise GDT
		 */
		void init();
		/**
			\brief Encodes a GDT entry descriptor into a physical entry of the GDT
			\param target Target region of the GDT
			\param source GDT entry descriptor to be encoded into the GDT
		 */
		void encodeEntry(uint8_t *target, GDT_entry source);
	private:
		/**
			\brief Load gdt register 
		 */
		void load_gdtr(gdtr_t gdtr);
		/**
			\brief GDT register
		 */
		gdtr_t gdtr;
		/**
			\brief Task State Segment
		 */
		uint8_t tss[TSS_SIZE];
		/**
			\brief Global Descriptor Table
		 */
		uint8_t gdt[GDT_SIZE];
		GDT();
};
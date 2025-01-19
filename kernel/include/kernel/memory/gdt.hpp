#pragma once
#ifdef __i386
#include "../arch/i386/defs/gdt/gdt_entry.h"
#include "../arch/i386/defs/gdt/gdtr.h"
#include "../arch/i386/defs/gdt/gdt_size.h"
#include "../arch/i386/defs/gdt/gdt_bits.hpp"
#endif
#include "tss.hpp"


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
			\param arg Target region of the GDT
			\param source GDT entry descriptor to be encoded into the GDT
		 */
		void encodeEntry(gdt_entry_bits *arg, struct GDT_entry source);

		void set_stack(uint32_t stack);
	private:
		/**
			\brief Load gdt register 
		 */
		void load_gdtr(gdtr_t gdtr);
		void write_tss(gdt_entry_bits *g);
		
		/**
			\brief GDT register
		 */
		gdtr_t gdtr;
		/**
			\brief Global Descriptor Table
		 */
		gdt_entry_bits gdt[6];
		GDT();
};
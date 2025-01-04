#include <kernel/memory/gdt.hpp>
#include <kernel/drivers/interrupts.hpp>
#include <kernel/kernel/panic.hpp>
#include <string.h>
#include <kernel/assembly/inlineasm.h>

extern "C" void reloadSegments();

GDT::GDT() {
	gdtr = {sizeof(gdt)-1, (uint32_t)gdt};
}

GDT &GDT::get() {
	static GDT instance;
	return instance;
}
extern "C" void flush_tss(void);

void GDT::init() {
	// First disable interrupts
	IDT::disable_interrupts();
	const struct GDT_entry null_entry = {0,0,0,0};
	const struct GDT_entry ker_cod_seg = {0xFFFFF, 0, 0x9A, 0xC};
	const struct GDT_entry ker_dat_seg = {0xFFFFF, 0, 0x92, 0xC};
	const struct GDT_entry usr_cod_seg = {0xFFFFF, 0, 0xFA, 0xC};
	const struct GDT_entry usr_dat_seg = {0xFFFFF, 0, 0xF2, 0xC};;

	encodeEntry(gdt, null_entry);
	encodeEntry(gdt + 1, ker_cod_seg);
	encodeEntry(gdt + 2, ker_dat_seg);
	encodeEntry(gdt + 3, usr_cod_seg);
	encodeEntry(gdt + 4, usr_dat_seg);

	gdt_entry_bits *ring3_code = &gdt[3];
	gdt_entry_bits *ring3_data = &gdt[4];

	ring3_code->limit_low = 0xFFFF;
	ring3_code->base_low = 0;
	ring3_code->accessed = 0;
	ring3_code->read_write = 1; // since this is a code segment, specifies that the segment is readable
	ring3_code->conforming_expand_down = 0; // does not matter for ring 3 as no lower privilege level exists
	ring3_code->code = 1;
	ring3_code->code_data_segment = 1;
	ring3_code->DPL = 3; // ring 3
	ring3_code->present = 1;
	ring3_code->limit_high = 0xF;
	ring3_code->available = 1;
	ring3_code->long_mode = 0;
	ring3_code->big = 1; // it's 32 bits
	ring3_code->gran = 1; // 4KB page addressing
	ring3_code->base_high = 0;

	*ring3_data = *ring3_code; // contents are s
	ring3_data->code = 0;

	write_tss(gdt + 5);
	
	load_gdtr(gdtr);
	reloadSegments();
	flush_tss();
}

void GDT::encodeEntry(gdt_entry_bits *arg, struct GDT_entry source) {
	if (source.limit > 0xFFFFF) {kn::panic("Source limit too large. Maximum is 0xFFFFF\n");}
	uint8_t * target = reinterpret_cast<uint8_t*>(arg);
	// Encode the limit
	target[0] = source.limit & 0xFF;
	target[1] = (source.limit >> 8) & 0xFF;
	target[6] = (source.limit >> 16) & 0x0F;
	
	// Encode the base
	target[2] = source.base & 0xFF;
	target[3] = (source.base >> 8) & 0xFF;
	target[4] = (source.base >> 16) & 0xFF;
	target[7] = (source.base >> 24) & 0xFF;
	
	// Encode the access byte
	target[5] = source.access_byte;
	
	// Encode the flags
	target[6] |= (source.flags << 4);

}

void GDT::load_gdtr(gdtr_t gdtr) {
	__asm__ __volatile__( "lgdt %0" :: "m"(gdtr) );
}

extern uint32_t stack_top;

void GDT::write_tss(gdt_entry_bits *g) {
	// Compute the base and limit of the TSS for use in the GDT entry.
	uint32_t base = (uint32_t) &tss;
	uint32_t limit = (sizeof tss);

	// Add a TSS descriptor to the GDT.
	g->limit_low = limit;
	g->base_low = base;
	g->accessed = 1; // With a system entry (`code_data_segment` = 0), 1 indicates TSS and 0 indicates LDT
	g->read_write = 0; // For a TSS, indicates busy (1) or not busy (0).
	g->conforming_expand_down = 0; // always 0 for TSS
	g->code = 1; // For a TSS, 1 indicates 32-bit (1) or 16-bit (0).
	g->code_data_segment=0; // indicates TSS/LDT (see also `accessed`)
	g->DPL = 0; // ring 0, see the comments below
	g->present = 1;
	g->limit_high = (limit & (0xf << 16)) >> 16; // isolate top nibble
	g->available = 0; // 0 for a TSS
	g->long_mode = 0;
	g->big = 0; // should leave zero according to manuals.
	g->gran = 0; // limit is in bytes, not pages
	g->base_high = (base & (0xff << 24)) >> 24; //isolate top byte

	// Ensure the TSS is initially zero'd.
	memset(&tss, 0, sizeof tss);

	tss.ss0  = 0x10;  // Set the kernel stack segment.
	tss.esp0 = get_esp(); // Set the kernel stack pointer.
	tss.iomap_base = sizeof tss;
	//note that CS is loaded from the IDT entry and should be the regular kernel code segment
}

void GDT::set_stack(uint32_t stack) {
	tss.esp0 = stack;
}
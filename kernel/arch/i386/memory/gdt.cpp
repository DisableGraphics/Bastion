#include <kernel/memory/gdt.hpp>
#include <kernel/drivers/interrupts.hpp>
#include <kernel/kernel/panic.hpp>

extern "C" void reloadSegments();

GDT::GDT() {
	gdtr = {sizeof(gdt)-1, (uint32_t)gdt};
}

GDT &GDT::get() {
	static GDT instance;
	return instance;
}

void GDT::init() {
	// First disable interrupts
	IDT::disable_interrupts();
	const struct GDT_entry null_entry = {0,0,0,0};
	const struct GDT_entry ker_cod_seg = {0xFFFFF, 0, 0x9A, 0xC};
	const struct GDT_entry ker_dat_seg = {0xFFFFF, 0, 0x92, 0xC};
	const struct GDT_entry usr_cod_seg = {0xFFFFF, 0, 0xFA, 0xC};
	const struct GDT_entry usr_dat_seg = {0xFFFFF, 0, 0xF2, 0xC};;
	const struct GDT_entry tss_seg = {sizeof(tss)-1,(uint32_t)&tss,0x89,0};

	encodeEntry(gdt, null_entry);
	encodeEntry(gdt + 0x8, ker_cod_seg);
	encodeEntry(gdt + 0x10, ker_dat_seg);
	encodeEntry(gdt + 0x18, usr_cod_seg);
	encodeEntry(gdt + 0x20, usr_dat_seg);
	encodeEntry(gdt + 0x28, tss_seg);
	load_gdtr(gdtr);
	reloadSegments();
}

void GDT::encodeEntry(uint8_t *target, struct GDT_entry source) {
	if (source.limit > 0xFFFFF) {kn::panic("Source limit too large. Maximum is 0xFFFFF\n");}
    
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
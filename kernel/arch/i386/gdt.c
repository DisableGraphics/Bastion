#include <kernel/gdt.h>
#include <error.h>

void init_gdt() {
	// First disable interrupts
	__asm__ __volatile__("cli");
	const struct GDT null_entry = {0,0,0,0};
	const struct GDT ker_cod_seg = {0xFFFFF, 0, 0x9A, 0xC};
	const struct GDT ker_dat_seg = {0xFFFFF, 0, 0x92, 0xC};
	const struct GDT usr_cod_seg = {0xFFFFF, 0, 0xFA, 0xC};
	const struct GDT usr_dat_seg = {0xFFFFF, 0, 0xF2, 0xC};;
	const struct GDT tss_seg = {0,0,0,0};

	encodeGdtEntry(gdt, null_entry);
	encodeGdtEntry(gdt + 8, ker_cod_seg);
	encodeGdtEntry(gdt + 16, ker_dat_seg);
	encodeGdtEntry(gdt + 24, usr_cod_seg);
	encodeGdtEntry(gdt + 32, usr_dat_seg);
	encodeGdtEntry(gdt + 40, tss_seg);

	const struct GDTR gdt_register = {48, (uint32_t)gdt};

	load_gtdr(gdt_register);
}

void encodeGdtEntry(uint8_t *target, struct GDT source) {
	if (source.limit > 0xFFFFF) {kerror("GDT cannot encode limits larger than 0xFFFFF. Limit: %d", source.limit);}
    
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

void load_gtdr(struct GDTR gdt_register)
{
    __asm__ __volatile__( "lgdtl %0" :: "m"(gdt_register) );
}
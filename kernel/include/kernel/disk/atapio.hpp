#pragma once
#include <kernel/assembly/inlineasm.h>
#ifdef __i386
#include "../arch/i386/defs/ata/cmd.hpp"
#include "../arch/i386/defs/ata/ports.hpp"
#include "../arch/i386/defs/ata/status_flags.hpp"
#endif

class ATA {
	public:
		static ATA &get();
		void init();
		bool identify();
		bool read_sector(uint32_t lba, uint16_t* buffer);
		bool write_sector(uint32_t lba, const uint16_t* buffer);
	private:
		bool wait_ready();
		uint16_t identity_buffer[256];
		ATA(){};
};
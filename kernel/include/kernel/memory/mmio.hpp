#pragma once
#include <stdint.h>

class MMIO
{
    public:
        static uint8_t read8 (void* p_address);
        static uint16_t read16 (void* p_address);
        static uint32_t read32 (void* p_address);
        static uint64_t read64 (void* p_address);
		template<typename T>
		static T read(void* p_address);
        static void write8 (void* p_address,uint8_t p_value);
        static void write16 (void* p_address,uint16_t p_value);
        static void write32 (void* p_address,uint32_t p_value);
        static void write64 (void* p_address,uint64_t p_value);
		template <typename T>
		void write(void* p_address, const T& p_value);
};

template <typename T>
T MMIO::read(void* p_address) {
	return *((volatile T*)(p_address));
}

template <typename T>
void MMIO::write(void* p_address, const T& p_value) {
	(*((volatile T*)(p_address)))=(p_value);
}
#include <kernel/memory/mmio.hpp>

uint8_t MMIO::read8 (void * p_address)
{
	return *((volatile uint8_t*)(p_address));
}
uint16_t MMIO::read16 (void * p_address)
{
	return *((volatile uint16_t*)(p_address));
	
}
uint32_t MMIO::read32 (void * p_address)
{
	return *((volatile uint32_t*)(p_address));
	
}
uint64_t MMIO::read64 (void * p_address)
{
	return *((volatile uint64_t*)(p_address));    
}
void MMIO::write8 (void * p_address,uint8_t p_value)
{
	(*((volatile uint8_t*)(p_address)))=(p_value);
}
void MMIO::write16 (void * p_address,uint16_t p_value)
{
	(*((volatile uint16_t*)(p_address)))=(p_value);    
}
void MMIO::write32 (void * p_address,uint32_t p_value)
{
	(*((volatile uint32_t*)(p_address)))=(p_value);
	
}
void MMIO::write64 (void * p_address,uint64_t p_value)
{
	(*((volatile uint64_t*)(p_address)))=(p_value);    
}

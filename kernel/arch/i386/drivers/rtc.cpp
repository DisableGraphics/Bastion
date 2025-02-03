#include <kernel/drivers/rtc.hpp>
#include <kernel/drivers/nmi.hpp>
#include <kernel/assembly/inlineasm.h>
#include <kernel/drivers/pic.hpp>
#include <kernel/bits/bits.hpp>
#include <kernel/time/time.hpp>

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

RTC::RTC() {

}

void RTC::init() {
	IDT::disable_interrupts();
	NMI::disable();
	
	outb(CMOS_ADDR, 0x8B);
	char prev = inb(CMOS_DATA);

	outb(CMOS_ADDR, 0x8B);

	outb(CMOS_DATA, prev | 0x10);
	
	read_register_c();

	basic_setup(hal::RTC);

	NMI::enable();
	IDT::enable_interrupts();
}

uint8_t RTC::get_register(int reg) {
      outb(CMOS_ADDR, reg);
      return inb(CMOS_DATA);
}

void RTC::compute_days() {
	total_days = TimeManager::days_until_year(year) + TimeManager::days_in_year_until_month_day(year, month, day);
}

uint8_t RTC::read_register_c() {
	outb(0x70, 0x0C);
	return inb(0x71);
}

void RTC::update_timestamp() {
	read_register_c();

	second = get_register(0x00);
    minute = get_register(0x02);
    hour = get_register(0x04);
	day = get_register(0x07);
    month = get_register(0x08);
    year = get_register(0x09);

	uint8_t register_b = get_register(0xB);

	if (!(register_b & 0x04)) {
		second = bits::bcd2normal(second);
		minute = bits::bcd2normal(minute);
		hour = ((hour & 0x0F) + (((hour & 0x70) / 16) * 10)) | (hour & 0x80);
		day = bits::bcd2normal(day);
		month = bits::bcd2normal(month);
		year = bits::bcd2normal(year) + RTC::century;
	}

	// 12 hour to 24 hour
	if (!(register_b & 0x02) && (hour & 0x80)) {
		hour = ((hour & 0x7F) + 12) % 24;
	}

	if(total_days != day) 
		compute_days();
	timestamp = total_days * 86400 + hour * 3600 + minute * 60 + second;
}
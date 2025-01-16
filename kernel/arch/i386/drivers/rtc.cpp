#include <kernel/drivers/rtc.hpp>
#include <kernel/drivers/nmi.hpp>
#include <kernel/assembly/inlineasm.h>
#include <kernel/drivers/pic.hpp>
#include <kernel/bits/bits.hpp>
#include <kernel/time/time.hpp>

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

RTC& RTC::get() {
	static RTC instance;
	return instance;
}

void RTC::init() {
	IDT::get().set_handler(PIC::get().get_offset() + 8, RTC::interrupt_handler);
	IDT::disable_interrupts();
	NMI::disable();
	
	outb(CMOS_ADDR, 0x8B);
	char prev = inb(CMOS_DATA);

	outb(CMOS_ADDR, 0x8B);

	outb(CMOS_DATA, prev | 0x10);
	
	read_register_c();

	PIC::get().IRQ_clear_mask(8);

	NMI::enable();
	IDT::enable_interrupts();
}

uint8_t RTC::get_register(int reg) {
      outb(CMOS_ADDR, reg);
      return inb(CMOS_DATA);
}

void RTC::interrupt_handler(interrupt_frame*) {
	IDT::disable_interrupts();
	RTC& rtc = RTC::get();
	rtc.read_register_c();

	rtc.second = rtc.get_register(0x00);
    rtc.minute = rtc.get_register(0x02);
    rtc.hour = rtc.get_register(0x04);
	rtc.day = rtc.get_register(0x07);
    rtc.month = rtc.get_register(0x08);
    rtc.year = rtc.get_register(0x09);

	uint8_t register_b = rtc.get_register(0xB);

	if (!(register_b & 0x04)) {
		rtc.second = bits::bcd2normal(rtc.second);
		rtc.minute = bits::bcd2normal(rtc.minute);
		rtc.hour = ((rtc.hour & 0x0F) + (((rtc.hour & 0x70) / 16) * 10)) | (rtc.hour & 0x80);
		rtc.day = bits::bcd2normal(rtc.day);
		rtc.month = bits::bcd2normal(rtc.month);
		rtc.year = bits::bcd2normal(rtc.year) + RTC::century;
	}

	// 12 hour to 24 hour
	if (!(register_b & 0x02) && (rtc.hour & 0x80)) {
		rtc.hour = ((rtc.hour & 0x7F) + 12) % 24;
	}

	TimeManager::get().set_time(rtc.get_timestamp());

	PIC::get().send_EOI(8);
	IDT::enable_interrupts();
}

void RTC::compute_days() {
	total_days = TimeManager::days_until_year(year) + TimeManager::days_in_year_until_month_day(year, month, day);
}

uint64_t RTC::get_timestamp() {
	if(total_days != day) 
		compute_days();
	return total_days * 86400 + hour * 3600 + minute * 60 + second;
}

uint8_t RTC::read_register_c() {
	outb(0x70, 0x0C);
	return inb(0x71);
}
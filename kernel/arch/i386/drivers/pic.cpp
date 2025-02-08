#include <stdint.h>
#include <kernel/drivers/pic.hpp>
#include <kernel/assembly/inlineasm.h>
#include <kernel/hal/managers/irqcmanager.hpp>
#include <kernel/kernel/log.hpp>

PIC::PIC() {
	init();
}

void PIC::init() {
	remap(0x20, 0x28);
	disable();
	// PIC has 16 lines
	irq_lines.reserve(16);
	for(size_t i = 0; i < 16; i++) {
		irq_lines.push_back({});
		hal::IRQControllerManager::get().set_irq_for_controller(this, i);
	}
}

void PIC::remap(int offset1, int offset2) {
	uint8_t a1, a2;
	
	a1 = inb(PIC1_DATA); // save masks
	a2 = inb(PIC2_DATA);
	
	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC1_DATA, offset1); // ICW2: Master PIC vector offset
	io_wait();
	outb(PIC2_DATA, offset2); // ICW2: Slave PIC vector offset
	io_wait();
	outb(PIC1_DATA, 4); // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	io_wait();
	outb(PIC2_DATA, 2); // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();
	
	outb(PIC1_DATA, ICW4_8086); // ICW4: have the PICs use 8086 mode (and not 8080 mode)
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();
	
	outb(PIC1_DATA, a1); // restore saved masks.
	outb(PIC2_DATA, a2);
}

void PIC::disable() {
	outb(PIC1_DATA, 0xff);
	outb(PIC2_DATA, 0xff);
}

void PIC::disable_irq(size_t IRQline) {
	uint16_t port;
	uint8_t value;

	if(IRQline < 8) {
		port = PIC1_DATA;
	} else {
		port = PIC2_DATA;
		IRQline -= 8;
	}
	value = inb(port) | (1 << IRQline);
	outb(port, value);
}

void PIC::enable_irq(size_t IRQline) {
	uint16_t port;
	uint8_t value;

	if(IRQline < 8) {
		port = PIC1_DATA;
	} else {
		port = PIC2_DATA;
		IRQline -= 8;
		enable_irq(2);
	}
	value = inb(port) & ~(1 << IRQline);
	outb(port, value);
}

void PIC::ack(size_t irqline) {
	// Does nothing, since acknowledgement is not something in the PIC world
}

uint16_t PIC::get_irq_reg(int ocw3) {
	/* OCW3 to PIC CMD to get the register values.  PIC2 is chained, and
	 * represents IRQs 8-15.  PIC1 is IRQs 0-7, with 2 being the chain */
	outb(PIC1_COMMAND, ocw3);
	outb(PIC2_COMMAND, ocw3);
	return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}

uint16_t PIC::get_irr() {
	return get_irq_reg(PIC_READ_IRR);
}

uint16_t PIC::get_isr() {
	return get_irq_reg(PIC_READ_ISR);
}

void PIC::eoi(size_t irq) {
	if (irq >= 8) {
		outb(PIC2_COMMAND, PIC_EOI);
	}
	outb(PIC1_COMMAND, PIC_EOI);
}

uint8_t PIC::get_offset() {
	return 0x20;
}

size_t PIC::get_default_irq(hal::Device dev) {
	switch(dev) {
		case hal::PIT:
			return 0;
		case hal::RTC:
			return 8;
		case hal::KEYBOARD:
			return 1;
		case hal::MOUSE:
			return 12;
		case hal::STORAGE:
			return -1;
		case hal::NETWORK:
			return -1;
		case hal::SCREEN:
			return -1;
	}
	return -1;
}

void PIC::register_driver(hal::Driver* driver, size_t irqline) {
	log(INFO, "PIC requested driver %p to line %d", driver, irqline);
	if(irqline < 16) {
		irq_lines[irqline].push_back(driver);
	}
}

size_t PIC::assign_irq(hal::Driver* device) {
	static size_t last_irq = -1;
	// Mandatorily reserved IRQs for the OS to work properly
	// PIT, Keyboard, RTC & mouse in that order
	const static size_t reserved_irqs[] = {0,1,8,12};
	last_irq++;
	switch (last_irq) {
		case 0:
		case 1:
			last_irq = 2;
			break;
		case 8:
			last_irq = 9;
			break;
		case 12:
			last_irq = 13;
			break;
	}
	return last_irq;
}
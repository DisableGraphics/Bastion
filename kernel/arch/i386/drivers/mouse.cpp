#include <kernel/drivers/mouse.hpp>
#include <kernel/drivers/pic.hpp>
#include <kernel/assembly/inlineasm.h>
#include "../defs/ps2/registers.h"
#include "kernel/drivers/ps2.hpp"
#include <stdio.h>

void Mouse::init() {
	port = PS2Controller::get().get_mouse_port();
	if(port == -1) return; // No mouse connected
	irqline = port == 1 ? 1 : 12;
	try_init_wheel();
	type = PS2Controller::get().get_device_type(port);
	switch(type) {
        case PS2Controller::MOUSE:
			nbytes = 3;
			break;
        case PS2Controller::MOUSE_SCROLLWHEEL:
			nbytes = 4;
			break;
        case PS2Controller::MOUSE_5BUTTON:
			break;
        default:
          	return;
	}

	IDT::get().set_handler(irqline + 0x20, Mouse::mouse_handler);
	if(irqline > 8)
		PIC::get().IRQ_clear_mask(2); // Slave PIC
	PIC::get().IRQ_clear_mask(irqline);
}

Mouse& Mouse::get() {
	static Mouse instance;
	return instance;
}

void Mouse::mouse_handler(interrupt_frame*) {
	IDT &idt = IDT::get();
	idt.disable_interrupts();
	Mouse &m = Mouse::get();
	uint8_t byte = inb(COMMAND_REGISTER);
	if(!(byte & 0x21)) goto finish;
	if(m.initialising) {
		do {
			byte = inb(DATA_PORT);
		} while(byte != 0xFA);
		m.initialising = false;
	} else {
		byte = inb(DATA_PORT);
		switch(m.state) {
			case FIRST_BYTE:
				m.cur_mouse_event.button_clicked = BTN_NONE;
				if(byte & 0x04) {
					m.cur_mouse_event.button_clicked |= MIDDLE_CLICK;
				}
				if(byte & 0x02) {
					m.cur_mouse_event.button_clicked |= RIGHT_CLICK;
				}
				if(byte & 0x01) {
					m.cur_mouse_event.button_clicked |= LEFT_CLICK;
				}
				if(byte & 0x20) {
					m.negy = true;
				}
				if(byte & 0x10) {
					m.negx = true;
				}
				break;
			case SECOND_BYTE:
				m.cur_mouse_event.xdisp = byte;
				if(m.negx)
					m.cur_mouse_event.xdisp |= 0xFF00;
				m.negx = false;
				break;
			case THIRD_BYTE:
				m.cur_mouse_event.ydisp = byte;
				if(m.negy)
					m.cur_mouse_event.ydisp |= 0xFF00;
				m.negy = false;
				if(m.nbytes == 3) {
					m.events_queue.push(m.cur_mouse_event);
				}
				break;
			case FOURTH_BYTE:
				m.cur_mouse_event.zdesp = byte;
				m.events_queue.push(m.cur_mouse_event);
				printf("Paice q va\n");
				break;
		}
		m.state = static_cast<MouseState>((m.state + 1) % m.nbytes);
	}
	
	finish:
	PIC::get().send_EOI(m.irqline);
	idt.enable_interrupts();
}

void Mouse::try_init_wheel() {
	PS2Controller &ps2 = PS2Controller::get();
	ps2.write_to_port(port, 0xF3);
	ps2.write_to_port(port, 200);
	uint8_t b = inb(DATA_PORT);
	printf("%p \n", b);
	ps2.write_to_port(port, 0xF3);
	ps2.write_to_port(port, 100);
	b = inb(DATA_PORT);
	printf("%p \n", b);
	ps2.write_to_port(port, 0xF3);
	ps2.write_to_port(port, 80);
	b = inb(DATA_PORT);
	printf("%p \n", b);

}
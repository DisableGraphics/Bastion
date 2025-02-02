#include <kernel/drivers/mouse.hpp>
#include <kernel/drivers/pic.hpp>
#include <kernel/assembly/inlineasm.h>
#include "../defs/ps2/registers.h"
#include "kernel/drivers/ps2.hpp"
#include <kernel/hal/managers/irqcmanager.hpp>

void PS2Mouse::init() {
	port = PS2Controller::get().get_mouse_port();
	if(port == -1) return; // No mouse connected
	
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

	basic_setup(hal::MOUSE);
}

void PS2Mouse::handle_interrupt() {
	uint8_t byte = inb(COMMAND_REGISTER);
	if(!(byte & 0x21)) return;
	if(initialising) {
		do {
			byte = inb(DATA_PORT);
		} while(byte != 0xFA);
		initialising = false;
	} else {
		byte = inb(DATA_PORT);
		switch(state) {
			case FIRST_BYTE:
				cur_mouse_event.button_clicked = BTN_NONE;
				if(byte & 0x04) {
					cur_mouse_event.button_clicked |= MIDDLE_CLICK;
				}
				if(byte & 0x02) {
					cur_mouse_event.button_clicked |= RIGHT_CLICK;
				}
				if(byte & 0x01) {
					cur_mouse_event.button_clicked |= LEFT_CLICK;
				}
				if(byte & 0x20) {
					negy = true;
				}
				if(byte & 0x10) {
					negx = true;
				}
				break;
			case SECOND_BYTE:
				cur_mouse_event.xdisp = byte;
				if(negx)
					cur_mouse_event.xdisp |= 0xFF00;
				negx = false;
				break;
			case THIRD_BYTE:
				cur_mouse_event.ydisp = byte;
				if(negy)
					cur_mouse_event.ydisp |= 0xFF00;
				negy = false;
				if(nbytes == 3) {
					events_queue.push(cur_mouse_event);
				}
				break;
			case FOURTH_BYTE:
				cur_mouse_event.zdesp = byte;
				events_queue.push(cur_mouse_event);
				break;
		}
		state = static_cast<MouseState>((state + 1) % nbytes);
	}
}

void PS2Mouse::try_init_wheel() {
	PS2Controller &ps2 = PS2Controller::get();
	ps2.write_to_port(port, 0xF3);
	inb(DATA_PORT);
	ps2.write_to_port(port, 200);
	inb(DATA_PORT);
	ps2.write_to_port(port, 0xF3);
	inb(DATA_PORT);
	ps2.write_to_port(port, 100);
	inb(DATA_PORT);
	ps2.write_to_port(port, 0xF3);
	inb(DATA_PORT);
	ps2.write_to_port(port, 80);
	inb(DATA_PORT);
}
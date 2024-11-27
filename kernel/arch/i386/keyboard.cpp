#include <kernel/keyboard.hpp>
#include <kernel/ps2.hpp>
#include <kernel/pic.hpp>
#include <kernel/interrupts.hpp>
#include <kernel/inlineasm.h>

#include "defs/ps2/registers.h"

#include <stdio.h>

#define MEDIA_KEY 0xE0
#define RELEASE_KEY 0xF0

Keyboard& Keyboard::get() {
	static Keyboard instance;
	return instance;
}

void Keyboard::init() {
	port = PS2Controller::get().get_keyboard_port();
	if(port == -1)
		return;

	irq_line = port == 2 ? 12 : 1;
	IDT::get().set_handler(irq_line + 0x20, Keyboard::keyboard_handler);
	if(irq_line > 8)
		PIC::get().IRQ_clear_mask(2); // Slave PIC
	PIC::get().IRQ_clear_mask(irq_line);
}

void Keyboard::keyboard_handler(interrupt_frame* a) {
	uint8_t recv = inb(DATA_PORT);
	// If ack to a command, ignore
	if(recv == 0xFA) {
		goto finish;
	}
	if(recv == MEDIA_KEY) {
		// Media key
		Keyboard::get().driver_state = MEDIA;
		goto finish;
	} else if(recv == RELEASE_KEY) {
		// Key has been released
		Keyboard::get().driver_state = RELEASE;
		goto finish;
	} else {
		// Everything else
		Keyboard::get().key_queue.push({
			Keyboard::get().driver_state == RELEASE ? true : false,
			get_key_from_bytes(recv)
		});
		if(Keyboard::get().driver_state != RELEASE)
			printf("%c", Keyboard::get().print_key());
		Keyboard::get().driver_state = NORMAL;
	}
finish:
	PIC::get().send_EOI(Keyboard::get().irq_line);
}

KEY Keyboard::get_key_from_bytes(uint8_t code) {
	switch(code) {
		case 0x01:
			return F9;
		case 0x03:
			return F5;
		case 0x04:
			return F3;
		case 0x05:
			return F1;
		case 0x06:
			return F2;
		case 0x07:
			return F12;
		case 0x09:
			return F10;
		case 0x0A:
			return F8;
		case 0x0B:
			return F6;
		case 0x0C:
			return F4;
		case 0x0D:
			return TAB;
		case 0x0E:
			return BACK_TICK;
		case 0x11:
			return LEFT_ALT;
		case 0x12:
			return LEFT_SHIFT;
		case 0x14:
			return LEFT_CTRL;
		case 0x15:
			return Q;
		case 0x16:
			return ONE;
		case 0x1A:
			return Z;
		case 0x1B:
			return S;
		case 0x1C:
			return A;
		case 0x1D:
			return W;
		case 0x1E:
			return TWO;
		case 0x21:
			return C;
		case 0x22:
			return X;
		case 0x23:
			return D;
		case 0x24:
			return E;
		case 0x25:
			return FOUR;
		case 0x26:
			return THREE;
		case 0x29:
			return SPACE;
		case 0x2A:
			return V;
		case 0x2B:
			return F;
		case 0x2C:
			return T;
		case 0x2D:
			return R;
		case 0x2E:
			return FIVE;
		case 0x31:
			return N;
		case 0x32:
			return B;
		case 0x33:
			return H;
		case 0x34:
			return G;
		case 0x35:
			return Y;
		case 0x36:
			return SIX;
		case 0x3A:
			return M;
		case 0x3B:
			return J;
		case 0x3C:
			return U;
		case 0x3D:
			return SEVEN;
		case 0x3E:
			return EIGHT;

		default:
			return NONE;
	}
	return NONE;
}

char Keyboard::print_key() {
	while(key_queue.size() > 0) {
		auto popped = key_queue.pop();
		if(!popped.released) return popped.key;
	}
	return 0;
}
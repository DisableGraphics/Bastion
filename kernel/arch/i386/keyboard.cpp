#include <kernel/keyboard.hpp>
#include <kernel/ps2.hpp>
#include <kernel/pic.hpp>
#include <kernel/interrupts.hpp>
#include <kernel/inlineasm.h>
#include <ctype.h>

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
		KEY data = get_key_from_bytes(recv);
		if(isupper(data)) data = static_cast<KEY>(tolower(data));
		Keyboard::get().key_queue.push({
			Keyboard::get().driver_state == RELEASE ? true : false,
			data
		});
		
		if(Keyboard::get().driver_state != RELEASE) {
			char c = Keyboard::get().poll_key();
			if(c)
				printf("%c", c);
		}
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
		case 0x41:
			return COMMA;
		case 0x42:
			return K;
		case 0x43:
			return I;
		case 0x44:
			return O;
		case 0x45:
			return ZERO;
		case 0x46:
			return NINE;
		case 0x49:
			return PERIOD;
		case 0x4A:
			return SLASH;
		case 0x4B:
			return L;
		case 0x4C:
			return SEMICOLON;
		case 0x4D:
			return P;
		case 0x4E:
			return MINUS;
		case 0x52:
			return QUOTE;
		case 0x54:
			return SQ_BRACKET_OPEN;
		case 0x55:
			return EQUALS;
		case 0x58:
			return CAPS_LOCK;
		case 0x59:
			return RIGHT_SHIFT;
		case 0x5A:
			return ENTER;
		case 0x5B:
			return SQ_BRACKET_CLOSE;
		case 0x5D:
			return BACKSLASH;
		case 0x66:
			return BACKSPACE;
		case 0x69:
			return ONE;
		case 0x6B:
			return FOUR;
		case 0x6C:
			return SEVEN;
		case 0x70:
			return ZERO;
		case 0x71:
			return PERIOD;
		case 0x72:
			return TWO;
		case 0x73:
			return FIVE;
		case 0x74:
			return SIX;
		case 0x75:
			return EIGHT;
		case 0x76:
			return ESCAPE;
		case 0x77:
			return NUM_LOCK;
		case 0x78:
			return F11;
		case 0x79:
			return PLUS;
		case 0x7A:
			return THREE;
		case 0x7B:
			return MINUS;
		case 0x7C:
			return ASTERISK;
		case 0x7D:
			return NINE;
		case 0x7E:
			return SCROLL_LOCK;
		case 0x83:
			return F7;

		default:
			return NONE;
	}
	return NONE;
}

char Keyboard::poll_key() {
	while(key_queue.size() > 0) {
		KEY_EVENT popped = key_queue.pop();
		update_key_flags(popped);
		if(!popped.is_special_key()) {
			if(!popped.released) {
				return get_char_with_flags(popped);
			}
		}
	}
	return 0;
}

void Keyboard::update_key_flags(const KEY_EVENT &event) {
	if(event.key == LEFT_SHIFT || event.key == RIGHT_SHIFT) {
		if(event.released)
			is_shift_pressed = false;
		else
			is_shift_pressed = true;
	}
	if(event.key == CAPS_LOCK) {
		if(event.released)
			caps_lock_active = !caps_lock_active;
	}
}

char Keyboard::get_char_with_flags(const KEY_EVENT &event) {
	if(caps_lock_active && is_shift_pressed)
		return event.key;
	if(caps_lock_active && islower(event.key))
		return toupper(event.key);
	if(is_shift_pressed)
		return event.get_with_shift();
	return event.key;
}
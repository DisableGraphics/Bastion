#include <stdint.h>
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
	char c;
	// If ack to a command, ignore
	if(recv == 0xFA) {
		goto finish;
	}
	Keyboard::get().driver_state = Keyboard::get().get_next_state(recv);
	switch(Keyboard::get().driver_state) {
        case INITIAL:
			break;
        case NORMAL_KEY_FINISHED:
			Keyboard::get().key_queue.push({
				Keyboard::get().is_key_released,
				get_key_normal(recv)
			});
			Keyboard::get().is_key_released = false;
			Keyboard::get().driver_state = INITIAL;
			break;
        case NORMAL_KEY_RELEASED:
			Keyboard::get().is_key_released = true;
			break;
        case MEDIA_KEY_RELEASED:
			Keyboard::get().is_key_released = true;
			break;
        case MEDIA_KEY_FINISHED:
			Keyboard::get().key_queue.push({
				Keyboard::get().is_key_released,
				get_key_media(recv)
			});
			Keyboard::get().is_key_released = false;
			Keyboard::get().driver_state = INITIAL;
			break;
        
        case PRINT_SCREEN_RELEASED:
			Keyboard::get().is_key_released = true;
        case PRINT_SCREEN_PRESSED:
			Keyboard::get().key_queue.push({
				Keyboard::get().is_key_released,
				PRINT_SCREEN
			});
			Keyboard::get().is_key_released = false;
			Keyboard::get().driver_state = INITIAL;
			break;
		/* Rubbish states that don't really need to do anything */
        case MEDIA_KEY_PRESSED:
		case PRINT_SCREEN_0x12:
		case PRINT_SCREEN_REL_0x7C:
        case PRINT_SCREEN_0xE0:
        case PRINT_SCREEN_REL_0xE0:
        case PRINT_SCREEN_REL_0xF0:
			break;
	}

	c = Keyboard::get().poll_key();
	if(c) { printf("%c", c); }

finish:
	PIC::get().send_EOI(Keyboard::get().irq_line);
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
	if(caps_lock_active && is_shift_pressed && isupper(event.key))
		return tolower(event.key);
	if(is_shift_pressed)
		return event.get_with_shift();
	if(caps_lock_active)
		return event.key;
	if(!is_shift_pressed && isupper(event.key))
		return tolower(event.key);
	
	return event.key;
}

Keyboard::STATE Keyboard::get_next_state(uint8_t recv) {
	if(driver_state == INITIAL) {
		switch(recv) {
			case MEDIA_KEY:
				return MEDIA_KEY_PRESSED;
			case RELEASE_KEY:
				return NORMAL_KEY_RELEASED;
			default:
				return NORMAL_KEY_FINISHED;
		}
	} else if(driver_state == MEDIA_KEY_PRESSED) {
		return INITIAL;
	} else if(driver_state == MEDIA_KEY_PRESSED) {
		switch(recv) {
			case 0x12:
				return PRINT_SCREEN_0x12;
			case RELEASE_KEY:
				return MEDIA_KEY_RELEASED;
			default:
				return MEDIA_KEY_FINISHED;
		}
	} else if(driver_state == NORMAL_KEY_RELEASED) {
		return NORMAL_KEY_FINISHED;
	} else if(driver_state == PRINT_SCREEN_0x12) {
		if(recv == 0xE0) return PRINT_SCREEN_0xE0;
	} else if(driver_state == MEDIA_KEY_RELEASED) {
		if(recv == 0x7C) return PRINT_SCREEN_REL_0x7C;
		return MEDIA_KEY_FINISHED;
	} else if(driver_state == MEDIA_KEY_FINISHED) {
		return INITIAL;
	} else if(driver_state == PRINT_SCREEN_REL_0x7C) {
		if(recv == 0xE0) return PRINT_SCREEN_REL_0xE0;
	} else if(driver_state == PRINT_SCREEN_0xE0) {
		if(recv == 0x7C) return PRINT_SCREEN_PRESSED;
	} else if(driver_state == PRINT_SCREEN_REL_0xE0) {
		if(recv == 0xF0) return PRINT_SCREEN_REL_0xF0;
	} else if(driver_state == PRINT_SCREEN_REL_0xF0) {
		if(recv == 0x12) return PRINT_SCREEN_RELEASED;
	}
	return INITIAL;
}
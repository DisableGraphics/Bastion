#include <kernel/drivers/keyboard/keyboard.hpp>
#include <kernel/drivers/ps2.hpp>
#include <kernel/drivers/pic.hpp>
#include <kernel/drivers/interrupts.hpp>
#include <kernel/assembly/inlineasm.h>

#include <kernel/hal/managers/irqcmanager.hpp>

#include "../../defs/ps2/registers.h"

#include <stdio.h>
#include <stdint.h>
#include <ctype.h>

#define MEDIA_KEY 0xE0
#define RELEASE_KEY 0xF0

PS2Keyboard& PS2Keyboard::get() {
	static PS2Keyboard instance;
	return instance;
}

void PS2Keyboard::init() {
	port = hal::PS2SubsystemController::get().get_keyboard_port();
	if(port == -1)
		return;

	basic_setup(port == 1 ? hal::KEYBOARD : hal::MOUSE);
}

void PS2Keyboard::handle_interrupt() {
	uint8_t recv = inb(DATA_PORT);
	char c;
	// If ack to a command, ignore
	if(recv == 0xFA) {
		goto finish;
	}
	
	driver_state = get_next_state(recv);
	switch(driver_state) {
		case NORMAL_KEY_FINISHED:
			key_queue.push({
				is_key_released,
				get_key_normal(recv)
			});
			is_key_released = false;
			driver_state = INITIAL;
			break;
		case NORMAL_KEY_RELEASED:
			is_key_released = true;
			break;
		case MEDIA_KEY_RELEASED:
			is_key_released = true;
			break;
		case MEDIA_KEY_FINISHED:
			key_queue.push({
				is_key_released,
				get_key_media(recv)
			});
			is_key_released = false;
			driver_state = INITIAL;
			break;
		case PRINT_SCREEN_RELEASED:
			is_key_released = true;
		case PRINT_SCREEN_PRESSED:
			key_queue.push({
				is_key_released,
				PRINT_SCREEN
			});
			is_key_released = false;
			driver_state = INITIAL;
			break;
		/* For all rubbish states that don't really need to do anything */
		default:
			break;
	}

	c = poll_key();
	if(c) { printf("%c", c); }

finish:
}

char PS2Keyboard::poll_key() {
	while(key_queue.size() > 0) {
		KEY_EVENT popped = key_queue.pop();
		update_key_flags(popped);
		if(!popped.is_special_key()) {
			if(!popped.released) {
				char c =  get_char_with_flags(popped);
				if(c) return c;
			}
		}
	}
	return 0;
}

void PS2Keyboard::update_key_flags(const KEY_EVENT &event) {
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
	if(event.key == NUM_LOCK) {
		if(event.released)
			num_lock_active = !num_lock_active;
	}
}

char PS2Keyboard::get_char_with_flags(const KEY_EVENT &event) {
	if(num_lock_active && event.is_numpad())
		return event.numpad_numlock();
	else if(is_shift_pressed && event.is_numpad())
		return event.numpad_numlock();
	else if(event.is_numpad()) {
		KEY e = event.numpad_no_numlock();
		if(!KEY_EVENT::is_special_key(e))
			return e;
		else return 0;
	}

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

PS2Keyboard::STATE PS2Keyboard::get_next_state(uint8_t recv) {
	switch(driver_state) {
		case INITIAL:
			switch(recv) {
				case MEDIA_KEY:
					return MEDIA_KEY_PRESSED;
				case RELEASE_KEY:
					return NORMAL_KEY_RELEASED;
				default:
					return NORMAL_KEY_FINISHED;
			}
		case NORMAL_KEY_FINISHED:
			return INITIAL;
		case MEDIA_KEY_PRESSED:
			switch(recv) {
				case 0x12:
					return PRINT_SCREEN_0x12;
				case RELEASE_KEY:
					return MEDIA_KEY_RELEASED;
				default:
					return MEDIA_KEY_FINISHED;
			}
		case NORMAL_KEY_RELEASED:
			return NORMAL_KEY_FINISHED;
		case PRINT_SCREEN_0x12:
			if(recv == 0xE0) return PRINT_SCREEN_0xE0;
			break;
		case MEDIA_KEY_RELEASED:
			if(recv == 0x7C) return PRINT_SCREEN_REL_0x7C;
			return MEDIA_KEY_FINISHED;
		case MEDIA_KEY_FINISHED:
			return INITIAL;
		case PRINT_SCREEN_REL_0x7C:
			if(recv == 0xE0) return PRINT_SCREEN_REL_0xE0;
			break;
		case PRINT_SCREEN_0xE0:
			if(recv == 0x7C) return PRINT_SCREEN_PRESSED;
			break;
		case PRINT_SCREEN_PRESSED:
			return INITIAL;
		case PRINT_SCREEN_REL_0xE0:
			if(recv == 0xF0) return PRINT_SCREEN_REL_0xF0;
			break;
		case PRINT_SCREEN_REL_0xF0:
			if(recv == 0x12) return PRINT_SCREEN_RELEASED;
			break;
		case PRINT_SCREEN_RELEASED:
			return INITIAL;
	}
	return INITIAL;
}
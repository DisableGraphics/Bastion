#include <stdint.h>
#include <kernel/hal/managers/ps2manager.hpp>
#include <kernel/assembly/inlineasm.h>
#include <kernel/kernel/panic.hpp>
#include <kernel/drivers/pit.hpp>
#include <kernel/hal/managers/timermanager.hpp>
#include <kernel/kernel/log.hpp>

#include "../defs/ps2/registers.h"
#include "../defs/ps2/commands.h"
#include "../defs/ps2/output_bits.h"
#include "../defs/ps2/return_command.h"
#include "../defs/ps2/config_byte.h"

hal::PS2SubsystemManager &hal::PS2SubsystemManager::get() {
	static hal::PS2SubsystemManager instance;
	return instance;
}

void hal::PS2SubsystemManager::init() {
	// Disable devices so it doesn't explode
	outb(COMMAND_REGISTER, DISABLE_FIRST_PORT);
	outb(COMMAND_REGISTER, DISABLE_SECOND_PORT);

	// Flush output buffer
	inb(DATA_PORT);

	configure_first_port();
	// Check if the controller is OK
	self_test();
	has_two_channels = two_channels();
	if(has_two_channels) {
		configure_second_port();
	}
	if(!test()) {
		kn::panic("PS2 Controller is broken.");
	}
	enable_devices();
	reset_and_detect_devices();
}

void hal::PS2SubsystemManager::configure_first_port() {
	// Set bits of the controller configuration byte
	outb(COMMAND_REGISTER, READ_BYTE_0);
	uint8_t ccb = inb(DATA_PORT);
	ccb &= ~FIRST_PORT_INTERRUPT; // Disable interrupts for first port
	ccb &= ~FIRST_PORT_TRANSLATION; // Disable translation for first port
	ccb &= ~FIRST_PORT_CLOCK; // Enable clock
	// Write the controller configuration byte to the controller
	outb(COMMAND_REGISTER, WRITE_BYTE_0);
	outb(DATA_PORT, ccb);
}

void hal::PS2SubsystemManager::self_test() {
	// Some computers reset their controllers when the controller is asked to do the self test.
	// so, to keep all the config, we load and then restore the controller config byte

	// Get config byte
	outb(COMMAND_REGISTER, READ_BYTE_0);
	uint8_t ccb = inb(DATA_PORT);
	// Perform self test
	outb(COMMAND_REGISTER, TEST_CONTROLLER);
	if(inb(DATA_PORT) != SELF_TEST_PASSED){
		kn::panic("PS2 controller failed self test. Can't continue booting.");
	}

	// Restore config byte
	outb(COMMAND_REGISTER, WRITE_BYTE_0);
	outb(DATA_PORT, ccb);
}

bool hal::PS2SubsystemManager::two_channels() {
	// Try to enable the second port
	outb(COMMAND_REGISTER, ENABLE_SECOND_PORT);

	// Check whether bit 5 is clear (If it is, there are two channels)
	outb(COMMAND_REGISTER, READ_BYTE_0);
	uint8_t ccb = inb(DATA_PORT);
	// Determine whether this controller has two channels
	return !(ccb & SECOND_PORT_CLOCK);
}

void hal::PS2SubsystemManager::configure_second_port() {
	// Disable second PS/2 port
	outb(COMMAND_REGISTER, DISABLE_SECOND_PORT);
	// Set bits of the controller configuration byte
	outb(COMMAND_REGISTER, READ_BYTE_0);
	uint8_t ccb = inb(DATA_PORT);
	ccb &= ~SECOND_PORT_INTERRUPT; // Disable interrupts for second port
	ccb &= ~SECOND_PORT_CLOCK; // Enable clock
	// Write the controller configuration byte to the controller
	outb(COMMAND_REGISTER, WRITE_BYTE_0);
	outb(DATA_PORT, ccb);
}

bool hal::PS2SubsystemManager::test() {
	bool are_tests_correct = true;

	// Test the first port
	outb(COMMAND_REGISTER, TEST_FIRST_PORT);
	// Returns 0 if ok
	if(inb(DATA_PORT)) {
		are_tests_correct = false;
		return are_tests_correct;
	}
	if(has_two_channels) {
		outb(COMMAND_REGISTER, TEST_SECOND_PORT);
		// Returns 0 if ok
		if(inb(DATA_PORT)) {
			are_tests_correct = false;
			return are_tests_correct;
		}
	}
	return are_tests_correct;
}

void hal::PS2SubsystemManager::enable_devices() {
	// Activate bits of interrupts
	outb(COMMAND_REGISTER, READ_BYTE_0);
	uint8_t ccb = inb(DATA_PORT);
	ccb |= FIRST_PORT_INTERRUPT;

	// Enable first port
	outb(COMMAND_REGISTER, ENABLE_FIRST_PORT);
	// if it has two channels, enable second port and its interrupts
	if(has_two_channels) {
		outb(COMMAND_REGISTER, ENABLE_SECOND_PORT);
		ccb |= SECOND_PORT_INTERRUPT;
	}
	outb(COMMAND_REGISTER, WRITE_BYTE_0);
	outb(DATA_PORT, ccb);
}

void hal::PS2SubsystemManager::reset_and_detect_devices() {
	for(int i = 0; i < (has_two_channels ? 2 : 1); i++) {
		int port = i+1;
		hal::TimerManager::get().exec_at(2*tc::s, on_timeout_expire, this);
		while(!timeout_expired) {
			uint8_t status_register = inb(STATUS_REGISTER);
			if(!(status_register & (1 << 1))) {
				break;
			}
		}
		if(timeout_expired) {
			log(INFO, "No device connected to port %d of PS/2 controller", port);
		} else {
			// Send reset command to the first port
			outb(DATA_PORT, RESET_DEVICE);
			uint8_t response = inb(DATA_PORT);
			if(response == 0xFA || response == 0xAA) {
				response = inb(DATA_PORT);
				if(response == 0xFA || response == 0xAA) {
					set_device_type(port, get_device_type(port));
				} else { 
					log(INFO, "Device in port %d of PS/2 controller is broken", port);
					set_device_type(port, NONE);
				}
			} else { 
				log(INFO, "Device in port %d of PS/2 controller is broken", port);
				set_device_type(port, NONE);
			}
		}
	}
}

void hal::PS2SubsystemManager::on_timeout_expire(volatile void* arg) {
	auto self = reinterpret_cast<volatile hal::PS2SubsystemManager*>(arg);
	self->timeout_expired = true;
}

hal::PS2SubsystemManager::DeviceType hal::PS2SubsystemManager::get_device_type(int port) {
	uint8_t response;
	PS2SubsystemManager::DeviceType device_type = NONE;
	switch(port) {
		case 2:
		case 1:
			// Disable scanning
			write_to_port(port, 0xF5);
			while(inb(DATA_PORT) != 0xFA);
			// Identify command
			write_to_port(port, 0xF2);
			while(inb(DATA_PORT) != 0xFA);
			response = inb(DATA_PORT);
			if(response == 0xAB || response == 0xAC) {
				log(INFO, "Keyboard connected to PS/2 port %d", port);
				response = inb(DATA_PORT);
				switch(response) {
					case 0x83:
					case 0xC1:
						device_type = MF2_KEYBOARD;
						break;
					case 0x84:
						device_type = SHORT_KEYBOARD;
						break;
					case 0x85:
						device_type = NCD_122_KEYBOARD;
						break;
					case 0x86:
						device_type = KEYBOARD_122_KEY;
						break;
					case 0x90:
						device_type = JP_G_KEYBOARD;
						break;
					case 0x91:
						device_type = JP_P_KEYBOARD;
						break;
					case 0x92:
						device_type = JP_A_KEYBOARD;
						break;
					case 0xA1:
						device_type = NCD_SUN_KEYBOARD;
						break;
					default:
						log(INFO, "wat: got %d", response);
						device_type = NONE;
						break;
				}
			} else if(response == 0x00 || response == 0x03 || response == 0x05) {
				log(INFO, "Mouse connected to port %d", port);
				device_type = static_cast<PS2SubsystemManager::DeviceType>(response);
			} else {
				log(INFO, "Unknown or broken. Received code: %p", response);
				device_type = NONE;
			}
			// Enable scanning again
			write_to_port(port, 0xF4);
		default:
			break;

	}
	return device_type;
}

void hal::PS2SubsystemManager::write_to_port(uint8_t port, uint8_t command) {
	if(port == 2) outb(COMMAND_REGISTER, WRITE_TO_PS2_2_IN_BUFFER);
	outb(DATA_PORT, command);
}

void hal::PS2SubsystemManager::set_device_type(int port, DeviceType type) {
	if(port == 1) {
		first_port_device = type;
	} else if(port == 2) {
		second_port_device = type;
	}
}

int hal::PS2SubsystemManager::get_keyboard_port() {
	if(first_port_device > 0x05) 
		return 1;
	if(has_two_channels) {
		if(second_port_device > 0x05)
			return 2;
		else 
			return -1;
	} else {
		return -1;
	}
}

int hal::PS2SubsystemManager::get_mouse_port() {
	if(first_port_device <= 0x05) 
		return 1;
	if(has_two_channels) {
		if(second_port_device <= 0x05)
			return 2;
		else 
			return -1;
	} else {
		return -1;
	}
}
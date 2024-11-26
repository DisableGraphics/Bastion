#include <kernel/ps2.hpp>
#include <kernel/inlineasm.h>
#include <kernel/panic.hpp>

#include <stdio.h>

#include "defs/ps2/registers.h"
#include "defs/ps2/commands.h"
#include "defs/ps2/output_bits.h"
#include "defs/ps2/return_command.h"
#include "defs/ps2/config_byte.h"

PS2Controller &PS2Controller::get() {
	static PS2Controller instance;
	return instance;
}

void PS2Controller::init() {
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
}

void PS2Controller::configure_first_port() {
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

void PS2Controller::self_test() {
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

bool PS2Controller::two_channels() {
	// Try to enable the second port
	outb(COMMAND_REGISTER, ENABLE_SECOND_PORT);

	// Check whether bit 5 is clear (If it is, there are two channels)
	outb(COMMAND_REGISTER, READ_BYTE_0);
	uint8_t ccb = inb(DATA_PORT);
	// Determine whether this controller has two channels
	return !(ccb & SECOND_PORT_CLOCK);
}

void PS2Controller::configure_second_port() {
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

bool PS2Controller::test() {
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

void PS2Controller::enable_devices() {
	// Enable devices
	outb(COMMAND_REGISTER, ENABLE_FIRST_PORT);
	if(has_two_channels)
		outb(COMMAND_REGISTER, ENABLE_SECOND_PORT);
}
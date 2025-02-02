#pragma once
#include <stddef.h>
#include <kernel/hal/device.hpp>
#include <kernel/datastr/vector.hpp>
#include <kernel/hal/drvbase/driver.hpp>

namespace hal {
	class IRQController {
		public:
			~IRQController() = default;
			/**
				\brief Enable IRQ line.
				\param irqline The IRQ line to enable.
			 */
			virtual void enable_irq(size_t irqline) = 0;
			virtual void disable_irq(size_t irqline) = 0;
			virtual void eoi(size_t irqline) = 0;
			virtual void ack(size_t irqline) = 0;

			virtual void disable() = 0;

			virtual void register_driver(hal::Driver* driver, size_t irqline) = 0;

			/**
				\brief Gets default IRQ line for a type of device.
				\details Implementation-defined.
				PIC should return the default IRQ line assigned to that type of device.
				-1 if there isn't really a IRQ line for that device in particular
				(E.g network cards)
				APIC should just return -1.
				\note If return value != -1, you should call IRQControllerManager::set_irq_for_controller
				to make sure the IRQControllerManager handles the IRQ correctly.
			 */
			virtual size_t get_default_irq(hal::Device device) = 0;
			/**
				\brief Assign first IRQ line for a device.
				\details Implementation-defined.
				PIC should return first free IRQ line not already assigned
				to a device, plus also keeping in mind all other devices
				of the OS. As in NEVER EVER return an IRQ line already in use
				by key hardware of the OS like the PIT, the keyboard, the mouse
				and/or the RTC (real time clock).
				APIC should only need to return first free IRQ line.
				\note Also call IRQControllerManager::set_irq_for_controller
				to make sure the IRQControllerManager handles the IRQ correctly.
				\warning NEVER EVER RETURN IRQ 96 FOR ANYTHING. IRQ 96 is
				RESERVED for system calls. Returning 96 will make drivers interrupt
				on the syscall vector and will make the system horribly confused at best
				and crash horribly/corrupt machine state at worst.
			 */
			virtual size_t assign_irq(hal::Driver* device) = 0;
			/**
				\brief Interrupt handler for the IRQ controller.
				\details Handles ALL devices connected to the IRQ line.
				Sends EOI on completion.
			 */
			virtual void interrupt_handler(size_t irqline);
		protected:
			/// Devices allocated for each irq line
			/// NOTE: Call reserve() on the vector when initialising 
			/// NOTE: irq_lines[i] -> Get devices connected to irq i
			/// NOTE: irq_lines[i][j] -> Get device j connected to irq i
			Vector<Vector<hal::Driver*>> irq_lines;
	};
}
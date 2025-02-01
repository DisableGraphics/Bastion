#pragma once
#include <stddef.h>
#include <kernel/hal/device.hpp>
#include <kernel/datastr/vector.hpp>
#include <kernel/hal/drvbase/driver.hpp>

namespace hal {
	template <size_t MaxIRQs, size_t MaxDevices>
	class IRQController {
		public:
			~IRQController() = default;

			virtual void enable_irq(size_t irqline) = 0;
			virtual void disable_irq(size_t irqline) = 0;
			virtual void eoi(size_t irqline) = 0;
			virtual void ack(size_t irqline) = 0;

			/**
				\brief Gets default IRQ line for a type of device.
				\details Implementation-defined.
				PIC should return the default IRQ line assigned to that type of device.
				-1 if there isn't really a IRQ line for that device in particular
				(E.g network cards)
				APIC should just return -1.
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
			 */
			virtual size_t assign_irq(hal::Device device) = 0;
		protected:
			// Devices allocated for each irq line
			Vector<hal::Driver*> irq_lines[MaxIRQs];
			// Maps each device to its IRQ line
			hal::Driver* device_irq_map[MaxDevices] = {};
	};
}
#pragma once

class NMI {
	public:
		/**
			\brief Disable Non Maskable Interrupts
			Required for critical sections of code
			that just can't be interrupted by any means
		 */
		static void disable();
		/**
			\brief Enable Non Maskable Interrupts
		 */
		static void enable();
	private:
};
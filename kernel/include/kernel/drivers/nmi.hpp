#pragma once
/**
	\brief Static class that controls whether
	Non-Maskable Interrupts are active or not.
	\note Necessary for the RTC (Real Time Clock)
	to be seup properly.
 */
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
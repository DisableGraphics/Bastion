#pragma once

class NMI {
	public:
		/**
			\brief Disable Non Maskable Interrupts
			Required for critical sections of code
			that just can't be interrupted by any means
		 */
		void disable();
		/**
			\brief Enable Non Maskable Interrupts
		 */
		void enable();
	private:
};

extern NMI nmi;
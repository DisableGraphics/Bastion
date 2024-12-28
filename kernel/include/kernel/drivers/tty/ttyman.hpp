#include <stdint.h>
#include <stddef.h>
#include <kernel/drivers/tty/tty.hpp>

/**
	\brief TTY Manager.

	Keeps an array of TTYs to display.
 */
class TTYManager {
	public:
		/**
			\brief Get singleton instance
		 */
		static TTYManager& get();
		/**
			\brief Initalisze the TTY Manager
		 */
		void init();
		/**
			\brief Write a char into the current TTY
			\param c Char to write
		 */
		void putchar(char c);
		/**
			\brief Write data into the current TTY
			\param data Data to write
			\param size Size of the data to write
		 */
		void write(const char* data, size_t size);
		/**
			\brief Write a null terminated string into the current TTY
			\param data String to write
		 */
		void writestring(const char* data);
		/**
			\brief Change current tty
			\param tty TTY to change. 
			If tty is more than the maximum number of ttys, 
			the last one is selected.
		 */
		void set_current_tty(size_t tty);
	private:
		/**
			\brief Copy the buffer of the current tty into
			the VGA's
		 */
		void display();
		TTYManager(){}
		// Current active TTY
		size_t current_tty = 0;
		// Maximum number of ttys
		const static size_t N_TTYS = 5;
		// Array of TTYs
		TTY ttys[N_TTYS];
		// VGA memory pointer
		uint16_t* const VGA_MEMORY = (uint16_t*) 0xC03FF000;
};
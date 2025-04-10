#include <stddef.h>
#include <kernel/drivers/tty/tty.hpp>
#include <kernel/hal/drvbase/video.hpp>

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
		void init(hal::VideoDriver* screen);
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
		/**
			\brief Clear current tty.
		*/
		void clear();
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
		size_t font_width, font_height, width, height, char_width, char_height, charsize;
		// Array of TTYs
		TTY ttys[N_TTYS];
		// Pointer to the screen handled by this manager
		hal::VideoDriver* screen;
};
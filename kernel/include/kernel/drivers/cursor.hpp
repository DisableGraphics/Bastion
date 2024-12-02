#include <stdint.h>
/**
	\brief Cursor driver.

	That blinking _ at the end of the text is this thing.
	Singleton.
 */
class Cursor {
	public:
		/**
			\brief Get singleton instance.
		 */
		static Cursor &get();
		/**
			\brief Initialise the cursor.

			Calls enable() under the hood.
		 */
		void init();
		/**
			\brief Enable the cursor and set its height
			\param cursor_start Pixels of height to begin
			\param cursor_end Pixels of height to end
		 */
		void enable(uint8_t cursor_start, uint8_t cursor_end);
		/**
			\brief Disable cursor
		 */
		void disable();
		/**
			\brief Move cursor to position (x, y)
		 */
		void move(int x, int y);
		/**
			\brief Return current cursor position.

			To get coordinates: y = pos / VGA_WIDTH; x = pos % VGA_WIDTH;
		 */
		uint16_t get_position();
	private:
		Cursor(){};
};
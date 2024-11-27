#include <stdint.h>
#include <kernel/interrupts.hpp>
#include <kernel/datastr/queue.hpp>
#include <kernel/mutex.hpp>
#include <kernel/key_event.hpp>
/**
	\brief Keyboard controller.
	Implemented as a singleton.
 */
class Keyboard {
	public:
		/**
			\brief Get singleton instance
		 */
		static Keyboard& get();
		/**
			\brief Initialise keyboard controller.

		 */
		void init();
	private:
		/**
			\brief Interrupt handler for keypresses.
		 */
		[[gnu::interrupt]]
		static void keyboard_handler(interrupt_frame * a);
		/**
			\brief Gets key value from scancode
		 */
		static KEY get_key_from_bytes(uint8_t one);
		/**
			\brief Get a key char from the queue.
			Only pressed keys will be gotten.
			Key flags (SHIFT, CAPS_LOCK...) will
			have their defined effect.
		 */
		char poll_key();
		/**
			\brief Updates key flags (SHIFT, CAPS_LOCK...).
		 */
		void update_key_flags(const KEY_EVENT &event);
		/**
			\brief Obtain character represented by event.
			Key flags will have their defined effect.
		 */
		char get_char_with_flags(const KEY_EVENT &event);
		/**
			\brief Queue for all key events
		 */
		StaticQueue<KEY_EVENT, 128> key_queue;
		// PS2 Controller port and IRQ line assignated to the
		// keyboard
		int port, irq_line;
		// State of the driver. 
		enum STATE {
			MEDIA,
			RELEASE,
			NORMAL
		} driver_state;
		// Flags
		// Is shift being pressed
		bool is_shift_pressed = false;
		// Is caps lock on
		bool caps_lock_active = false;

		Keyboard(){}
};
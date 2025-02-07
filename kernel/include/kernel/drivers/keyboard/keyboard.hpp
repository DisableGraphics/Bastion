#include <stdint.h>
#include <kernel/drivers/interrupts.hpp>
#include <kernel/datastr/queue.hpp>
#include <kernel/drivers/keyboard/key_event.hpp>

#include <kernel/hal/drvbase/keyboard.hpp>
/**
	\brief Keyboard controller.
	Implemented as a singleton.
 */
class PS2Keyboard : public hal::KeyboardDriver {
	public:
		/**
			\brief Get singleton instance
		 */
		static PS2Keyboard& get();
		/**
			\brief Initialise keyboard controller.

		 */
		void init() override;

		void handle_interrupt() override;
	private:
		enum STATE {
			INITIAL,
			NORMAL_KEY_FINISHED,
			MEDIA_KEY_PRESSED,
			NORMAL_KEY_RELEASED,
			PRINT_SCREEN_0x12,
			MEDIA_KEY_RELEASED,
			MEDIA_KEY_FINISHED,
			PRINT_SCREEN_REL_0x7C,
			PRINT_SCREEN_0xE0,
			PRINT_SCREEN_PRESSED,
			PRINT_SCREEN_REL_0xE0,
			PRINT_SCREEN_REL_0xF0,
			PRINT_SCREEN_RELEASED
		};

		/**
			\brief Gets key value from scancode. 
			Used for the states from the automaton:
			NORMAL_KEY_FINISHED.
			Returns a normal key.
		 */
		static KEY get_key_normal(uint8_t code);
		/**
			\brief Gets key value from scancode.
			Used for states of the automaton:
			MEDIA_KEY_FINISHED.
			Returns a media key-
		 */
		static KEY get_key_media(uint8_t code);
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
			\brief Get next state from the scancode received from the keyboard
		 */
		STATE get_next_state(uint8_t received);
		/**
			\brief Queue for all key events
		 */
		StaticQueue<KEY_EVENT, 128> key_queue;
		// PS2 Controller port and IRQ line assignated to the
		// keyboard
		int port = 0;
		// State of the driver. 
		STATE driver_state = INITIAL;
		// State flags
		bool is_key_released = false;

		// Flags
		// Is shift being pressed
		bool is_shift_pressed = false;
		// Is caps lock on
		bool caps_lock_active = false;
		// Is num lock on
		bool num_lock_active = false;
};
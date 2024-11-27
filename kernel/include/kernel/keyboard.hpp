#include <stdint.h>
#include <kernel/interrupts.hpp>
#include <kernel/datastr/queue.hpp>
#include <kernel/key.hpp>
#include <kernel/mutex.hpp>

struct KEY_EVENT {
	bool released;
	KEY key;
	bool is_special_key() {
		return key == ESCAPE ||
			key == CAPS_LOCK ||
			key == LEFT_SHIFT ||
			key == RIGHT_SHIFT ||
			key == LEFT_CTRL ||
			key == RIGHT_CTRL ||
			key == LEFT_WIN ||
			key == FN ||
			key == LEFT_ALT ||
			key == ALT_GR ||
			key == RIGHT_CTRL ||
			key == BEGIN ||
			key == END ||
			key == NEXT_PAGE ||
			key == PREV_PAGE ||
			key == NUM_LOCK ||
			key == SCROLL_LOCK ||
			(key >= F1 && key <= F12);
	}
};

class Keyboard {
	public:
		static Keyboard& get();
		void init();
		
	private:
		[[gnu::interrupt]]
		static void keyboard_handler(interrupt_frame * a);
		int port, irq_line;
		static KEY get_key_from_bytes(uint8_t one);
		StaticQueue<KEY_EVENT, 128> key_queue;
		char print_key();
		enum STATE {
			MEDIA,
			RELEASE,
			NORMAL
		} driver_state;
		bool uppercase = false;

		Keyboard(){}
};
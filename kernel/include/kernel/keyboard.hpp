#include <stdint.h>
#include <kernel/interrupts.hpp>
#include <kernel/datastr/queue.hpp>
#include <kernel/key.hpp>
#include <kernel/mutex.hpp>

struct KEY_EVENT {
	bool released;
	KEY key;
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

		Keyboard(){}
};
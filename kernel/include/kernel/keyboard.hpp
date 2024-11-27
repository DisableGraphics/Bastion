#include <stdint.h>
#include <kernel/interrupts.hpp>
#include <kernel/datastr/queue.hpp>
#include <kernel/key.hpp>
#include <kernel/mutex.hpp>
#include <ctype.h>

struct KEY_EVENT {
	bool released;
	KEY key;
	bool is_special_key() const {
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
	char get_with_shift() const {
		char c = static_cast<char>(key);
		if(isupper(c))
			return tolower(c);
		if(islower(c))
			return toupper(c);
		switch(key) {
			case BACK_TICK:
				return '~';
			case COMMA:
				return '<';
			case PERIOD:
				return '>';
			case SLASH:
				return '?';
			case SEMICOLON:
				return ':';
			case MINUS:
				return '_';
			case QUOTE:
				return '"';
			case SQ_BRACKET_OPEN:
				return '{';
			case SQ_BRACKET_CLOSE:
				return '}';
			case EQUALS:
				return '+';
			case BACKSLASH:
				return '|';
			case ONE:
				return '!';
			case TWO:
				return '@';
			case THREE:
				return '#';
			case FOUR:
				return '$';
			case FIVE:
				return '%';
			case SIX:
				return '^';
			case SEVEN:
				return '&';
			case EIGHT:
				return '*';
			case NINE:
				return '(';
			case ZERO:
				return ')';
			default:
				break;
		}
		return key;
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
		bool is_shift_pressed = false;
		bool caps_lock_active = false;
		void update_key_flags(const KEY_EVENT &event);
		char get_char_with_flags(const KEY_EVENT &event);

		Keyboard(){}
};
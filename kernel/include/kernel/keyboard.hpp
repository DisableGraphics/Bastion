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
				return TILDE;
			case COMMA:
				return LESS_THAN;
			case PERIOD:
				return MORE_THAN;
			case SLASH:
				return QUESTION_MARK;
			case SEMICOLON:
				return COLON;
			case MINUS:
				return UNDERSCORE;
			case QUOTE:
				return DOUBLE_QUOTE;
			case SQ_BRACKET_OPEN:
				return BRACKET_OPEN;
			case SQ_BRACKET_CLOSE:
				return BRACKET_CLOSE;
			case EQUALS:
				return PLUS;
			case BACKSLASH:
				return PIPE;
			case ONE:
				return EXCLAMATION_MARK;
			case TWO:
				return AT;
			case THREE:
				return HASH;
			case FOUR:
				return DOLLAR;
			case FIVE:
				return PERCENTAGE;
			case SIX:
				return CARET;
			case SEVEN:
				return AMPERSAND;
			case EIGHT:
				return ASTERISK;
			case NINE:
				return PAREN_OPEN;
			case ZERO:
				return PAREN_CLOSE;
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
#include <kernel/drivers/keyboard/key.hpp>
#include <kernel/drivers/keyboard/key_event.hpp>
#include <ctype.h>

bool KEY_EVENT::is_special_key(KEY key) {
	return key >= F1 && key < KEYP_ONE;
}

bool KEY_EVENT::is_special_key() const {
	return is_special_key(key);
}

bool KEY_EVENT::is_numpad() const {
	return key >= KEYP_ONE;
}

char KEY_EVENT::get_with_shift() const {
	char c = static_cast<char>(key);
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

KEY KEY_EVENT::numpad_numlock() const {
	switch(key) {
		case KEYP_ONE:
			return ONE;
		case KEYP_TWO:
			return TWO;
		case KEYP_THREE:
			return THREE;
		case KEYP_FOUR:
			return FOUR;
		case KEYP_FIVE:
			return FIVE;
		case KEYP_SIX:
			return SIX;
		case KEYP_SEVEN:
			return SEVEN;
		case KEYP_EIGHT:
			return EIGHT;
		case KEYP_NINE:
			return NINE;
		case KEYP_ZERO:
			return ZERO;
		case KEYP_ENTER:
			return ENTER;
		case KEYP_MINUS:
			return MINUS;
		case KEYP_PERIOD:
			return PERIOD;
		case KEYP_PLUS:
			return PLUS;
		case KEYP_SLASH:
			return SLASH;
		case KEYP_ASTERISK:
			return ASTERISK;
		default: break; 
	}
	return key;
}

KEY KEY_EVENT::numpad_no_numlock() const {
	switch(key) {
		case KEYP_ONE:
			return END;
		case KEYP_TWO:
			return CURSOR_DOWN;
		case KEYP_THREE:
			return PAGE_DOWN;
		case KEYP_FOUR:
			return CURSOR_LEFT;
		case KEYP_FIVE:
			return NONE;
		case KEYP_SIX:
			return CURSOR_RIGHT;
		case KEYP_SEVEN:
			return HOME;
		case KEYP_EIGHT:
			return CURSOR_UP;
		case KEYP_NINE:
			return PAGE_UP;
		case KEYP_ZERO:
			return INSERT;
		case KEYP_ENTER:
			return ENTER;
		case KEYP_MINUS:
			return MINUS;
		case KEYP_PERIOD:
			return DELETE;
		case KEYP_PLUS:
			return PLUS;
		case KEYP_SLASH:
			return SLASH;
		case KEYP_ASTERISK:
			return ASTERISK;
		default: break; 
	}
	return key;
}
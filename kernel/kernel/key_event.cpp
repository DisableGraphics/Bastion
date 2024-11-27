#include <kernel/key_event.hpp>
#include <ctype.h>

bool KEY_EVENT::is_special_key() const {
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

char KEY_EVENT::get_with_shift() const {
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
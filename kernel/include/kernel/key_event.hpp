#include <kernel/key.hpp>

struct KEY_EVENT {
	bool released;
	KEY key;
	bool is_special_key() const;
	bool is_numpad() const;
	char get_with_shift() const;

	static bool is_special_key(KEY key);
	KEY numpad_numlock() const;
	KEY numpad_no_numlock() const;
};
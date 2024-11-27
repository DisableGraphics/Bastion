#include <kernel/key.hpp>

struct KEY_EVENT {
	bool released;
	KEY key;
	bool is_special_key() const;
	char get_with_shift() const;
};
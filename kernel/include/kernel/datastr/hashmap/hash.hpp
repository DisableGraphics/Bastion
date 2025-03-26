#pragma once
#include <stddef.h>

template<typename T>
class Hash {
	size_t operator()(const T& key) {
		return reinterpret_cast<size_t>(key);
	}
};
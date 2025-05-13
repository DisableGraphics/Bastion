#pragma once
#include <stddef.h>
#include <string.h>

template<typename T>
struct Hash {
	size_t operator()(const T& key) {
		return static_cast<size_t>(key);
	}
};

template<>
struct Hash<char*> {
	size_t operator()(const char* const& key) {
		const size_t len = strlen(key);
		size_t hash = 0;
		for(size_t i = 0; i < len; i++) {
			hash += i;
			hash = (hash << 8) | (hash >> ((sizeof(size_t) << 3) - 8));
		}
		return hash;
	}
};
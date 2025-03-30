#pragma once
#include <stdint.h>

namespace tc {
	typedef uint32_t timertime;
	constexpr timertime s = 1000000;
	constexpr timertime ms = 1000;
	constexpr timertime us = 1;
}
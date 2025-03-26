#pragma once
#include "remove_reference.hpp"

template <typename T>
typename remove_reference<T>::type&& move(T&& t) noexcept {
	return static_cast<typename remove_reference<T>::type&&>(t);
}
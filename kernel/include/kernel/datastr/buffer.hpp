#pragma once
#include <stddef.h>

/**
	\brief RAII fixed size buffer with pointer semantics
*/
template<typename T>
class Buffer {
	public:
		Buffer(unsigned int size);
		~Buffer();

		size_t get_size();
		T* get_data();

		operator T*();

		static Buffer<T> make_buffer(T* data, unsigned int size);
	private:
		unsigned int size;
		T* data;
};

template <typename T>
Buffer<T>::Buffer(unsigned int size) {
	this->size = size;
	this->data = new T[size];
}

template <typename T>
Buffer<T>::~Buffer() {
	delete[] data;
}

template <typename T>
size_t Buffer<T>::get_size() {
	return size;
}

template <typename T>
T* Buffer<T>::get_data() {
	return data;
}

template <typename T>
Buffer<T>::operator T*() {
	return data;
}

template <typename T>
Buffer<T> Buffer<T>::make_buffer(T* data, unsigned int size) {
	Buffer<T> buffer(0);
	buffer.data = data;
	buffer.size = size;
	return buffer;
}
#pragma once
#include <stddef.h>
#include <stdlib.h>
#include <kernel/kernel/utility.hpp>

template <typename T>
class Vector {
	public:
		Vector(){ arr = reinterpret_cast<T*>(kmalloc(sizeof(T)*alloc_size)); };
		~Vector() { kfree(arr); }
		
		size_t size() const;
		void push_back(const T& elem);
		void emplace_back(T&& args);
		void pop_back();
		bool empty() const;

		void reserve(size_t n);
		
		T &back();
		T &operator[](size_t index);
		const T &operator[](size_t index) const;
	private:
		T *arr;
		size_t arrsize = 0;
		size_t alloc_size = 8;
};

template<typename T>
size_t Vector<T>::size() const {
	return arrsize;
}

template <typename T>
void Vector<T>::push_back(const T&elem) {
	if(arrsize == alloc_size) {
		alloc_size *= 2;
		void * tmp = krealloc(arr, alloc_size*sizeof(T));
		if(tmp) {
			arr = reinterpret_cast<T*>(tmp);
		} else {
			return;
		}
	}
	arr[arrsize++] = elem;
}

template <typename T>
void Vector<T>::pop_back() {
	T elem = arr[0];
	for(size_t i = 0; i < size(); i++) {
		arr[i] = arr[i+1];
	}
	arrsize--;
	return elem;
}

template <typename T>
T& Vector<T>::back() {
	return arr[arrsize-1];
}

template <typename T>
void Vector<T>::reserve(size_t n) {
	void * tmp = krealloc(arr, n*sizeof(T));
	if(tmp)
		arr = reinterpret_cast<T*>(tmp);
}

template<typename T>
T& Vector<T>::operator[](size_t index) {
	return arr[index];
}

template<typename T>
const T& Vector<T>::operator[](size_t index) const {
	return arr[index];
}

template <typename T>
void Vector<T>::emplace_back(T&& elem) {
	if(arrsize == alloc_size) {
		alloc_size *= 2;
		void * tmp = krealloc(arr, alloc_size*sizeof(T));
		if(tmp) {
			arr = reinterpret_cast<T*>(tmp);
		} else {
			return;
		}
	}
	arr[arrsize++] = move(elem);
}

template<typename T>
bool Vector<T>::empty() const {
	return size() == 0;
}
#pragma once
#include <stddef.h>
#include <stdlib.h>

template <typename T>
class Vector {
	public:
		Vector(){ arr = reinterpret_cast<T*>(kmalloc(sizeof(T)*alloc_size)); };
		~Vector() { kfree(arr); }
		
		size_t size();
		void push_back(const T& elem);
		void pop_back();

		void reserve(size_t n);
		
		T &back();
		T &operator[](size_t index);
	private:
		T *arr;
		size_t arrsize = 0;
		size_t alloc_size = 8;
};

template<typename T>
size_t Vector<T>::size() {
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
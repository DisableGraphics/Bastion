#pragma once
#include <stddef.h>
#include <stdlib.h>
#include <kernel/cpp/move.hpp>
#include <string.h>

/**
	\brief Dynamic array similar to std::vector.
	\warning Pointers to positions of this vector aren't guaranteed to be the same
	after a push_back() or an emplace_back() since they may call realloc().
 */
template <typename T>
class Vector {
	public:
		/**
			\brief Basic constructor
		 */
		Vector(){ arr = reinterpret_cast<T*>(kmalloc(sizeof(T)*alloc_size)); };
		/**
			\brief Constructor that reserves elements
		*/
		Vector(size_t nelem) { alloc_size = nelem; arr = reinterpret_cast<T*>(kmalloc(sizeof(T)*alloc_size)); }
		/**
			\brief Destructor. Frees resources.
		 */
		~Vector() { kfree(arr); }
		/**
			\brief Current size of the vector
		 */
		size_t size() const;
		/**
			\brief Push an element to the vector
			\details The element is put at the back of the vector.
			Has to realloc() array if there isn't enough space.
		 */
		void push_back(const T& elem);
		/**
			\brief Emplace using moves an element to the vector.
			\details The element is put at the back of the vector.
			Has to realloc() array if there isn't enough space.
		 */
		void emplace_back(T&& args);
		/**
			\brief Remove last element from the vector
		 */
		void pop_back();
		/**
			\brief Returns whether the vector is empty
			\returns If the vector is empty (true) or not (false)
		 */
		bool empty() const;
		/**
			\brief Erases element at position pos
			\details Calls destructor of deleted element
		 */
		void erase(size_t pos);
		/**
			\brief Reserve n elements of space.
			\note Useful for reserving a lot of space in one go.
		 */
		void reserve(size_t n);
		/**
			\brief Returns last element in the vector
			\returns Last element in the vector
		 */
		T &back();
		/**
			\brief Array access operator
		 */
		T &operator[](size_t index);
		/**
			\brief Const array access operator
		 */
		const T &operator[](size_t index) const;
	private:
		/// Internal pointer to the array
		T *arr;
		/// Size of array that's in use
		size_t arrsize = 0;
		/// Reserved size of array
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
	arrsize--;
}

template <typename T>
void Vector<T>::erase(size_t pos) {
	T elem = arr[pos];
	memmove(arr + pos, arr + pos + 1, (arrsize - pos - 1) * sizeof(T));
	arrsize--;
}

template <typename T>
T& Vector<T>::back() {
	return arr[arrsize-1];
}

template <typename T>
void Vector<T>::reserve(size_t n) {
	void * tmp = krealloc(arr, n*sizeof(T));
	if(tmp) {
		arr = reinterpret_cast<T*>(tmp);
		alloc_size = n;
	}
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
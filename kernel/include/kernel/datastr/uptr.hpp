#pragma once

template <typename T>
class UniquePtr {
	public:
		UniquePtr();
		UniquePtr(T *ptr);

		T &operator*();

		~UniquePtr();
	private:
		T * ptr;
};

template<typename T> 
UniquePtr<T>::UniquePtr() {
	ptr = new T();
}

template<typename T> 
UniquePtr<T>::UniquePtr(T *ptr) {
	this->ptr = ptr;
}

template<typename T> 
UniquePtr<T>::~UniquePtr() {
	delete ptr;
}

template<typename T> 
T& UniquePtr<T>::operator*() {
	return *ptr;
}
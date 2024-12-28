#pragma once

template <typename T>
class UniquePtr {
	public:
		UniquePtr();
		UniquePtr(T *ptr);
		UniquePtr(UniquePtr<T> &&other);

		T &operator*();
		T *operator->();
		UniquePtr<T>& operator=(UniquePtr<T>&& other);

		~UniquePtr();
	private:
		void move(UniquePtr<T> &&other) {
			this->ptr = other.ptr;
			other.ptr = nullptr;
		}
		UniquePtr(const UniquePtr<T> &other) = delete;
		void operator=(const UniquePtr<T> &other) = delete;
		T * ptr;
};

template<typename T> 
UniquePtr<T>::UniquePtr() {
	ptr = new T();
}

template<typename T> 
UniquePtr<T>::UniquePtr(UniquePtr<T> &&other) {
	move(other);
}

template<typename T> 
UniquePtr<T>& UniquePtr<T>::operator=(UniquePtr<T>&& other) {
	if (this != &other) {
		delete ptr;        // Clean up existing resource
		ptr = other.ptr;  // Transfer ownership
		other.ptr = nullptr; // Null out the source pointer
	}
	return *this;
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

template<typename T> 
T* UniquePtr<T>::operator->() {
	return ptr;
}
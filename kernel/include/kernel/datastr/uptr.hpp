#pragma once
/**
	\brief Unique Ptr that simulates an owning pointer
 */
template <typename T>
class UniquePtr {
	public:
		/**
			\brief Constructor. Creates a new object.
		 */
		UniquePtr();
		/**
			\brief Constructor. Reuses an already created pointer. 
			The pointer has to be allocated with new.
		 */
		UniquePtr(T *ptr);
		/**
			\brief Constructor. Creates a new constructor from another UniquePtr
		 */
		UniquePtr(UniquePtr<T> &&other);
		/// Get this uniqueptr's pointer
		T* get();
		/// Dereference operator
		T &operator*();
		/// Pointer attribute accessing operator
		T *operator->();
		/// Move assignation operator
		UniquePtr<T>& operator=(UniquePtr<T>&& other);
		/// Destructor
		~UniquePtr();
	private:
		void move(UniquePtr<T> &&other) {
			delete ptr;        // Clean up existing resource
			ptr = other.ptr;  // Transfer ownership
			other.ptr = nullptr; // Null out the source pointer
		}
		/// Disable copy constructor and copy assignment operator
		UniquePtr(const UniquePtr<T> &other) = delete;
		void operator=(const UniquePtr<T> &other) = delete;
		/// Internal pointer
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
		move(other);
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

template<typename T> 
T* UniquePtr<T>::get() {
	return ptr;
}
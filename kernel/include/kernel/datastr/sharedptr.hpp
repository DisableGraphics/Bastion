#include <stddef.h>
/**
	\brief Shared Pointer similar to std::shared_ptr<T>
	\warning This class isn't thread-safe unlike shared_ptr<T>
 */
template <typename T>
class SharedPtr {
	public:
		/// Default constructor
		SharedPtr() : ptr(nullptr), ref_count(nullptr) {}
		/// Constructor with raw pointer
		explicit SharedPtr(T* raw_ptr) : ptr(raw_ptr), ref_count(new size_t(1)) {}
		// Copy constructor
		SharedPtr(const SharedPtr& other);
		/// Move constructor
		SharedPtr(SharedPtr&& other) noexcept;

		/// Copy assignment operator
		SharedPtr& operator=(const SharedPtr& other);
		/// Move assignment operator
		SharedPtr& operator=(SharedPtr&& other) noexcept;

		/// Destructor
		~SharedPtr();

		/// Accessors
		T* get() const;
		/// Dereference operator
		T& operator*() const;
		/// Pointer attribute accessing operator
		T* operator->() const;
		/// How many pointers are active for the same resource
		size_t use_count() const;
		/// Convert to bool for checks
		explicit operator bool() const;
	private:
		T* ptr;                  // Raw pointer to the managed object
		size_t* ref_count;       // Reference count
		/// Release pointer
		void release();
};
template<typename T>
SharedPtr<T>::SharedPtr(const SharedPtr<T>& other) : ptr(other.ptr), ref_count(other.ref_count) {
	if (ref_count) {
		++(*ref_count);
	}
}

template <typename T>
SharedPtr<T>::SharedPtr(SharedPtr<T>&& other) noexcept : ptr(other.ptr), ref_count(other.ref_count) {
	other.ptr = nullptr;
	other.ref_count = nullptr;
}

template <typename T>
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr<T>& other) {
	if (this != &other) {
		release();
		ptr = other.ptr;
		ref_count = other.ref_count;
		if (ref_count) {
			++(*ref_count);
		}
	}
	return *this;
}

template <typename T>
SharedPtr<T>& SharedPtr<T>::operator=(SharedPtr<T>&& other) noexcept {
	if (this != &other) {
		release();
		ptr = other.ptr;
		ref_count = other.ref_count;
		other.ptr = nullptr;
		other.ref_count = nullptr;
	}
	return *this;
}

template <typename T>
SharedPtr<T>::~SharedPtr<T>() {
	release();
}

template<typename T>
T* SharedPtr<T>::get() const {
	return ptr;
}

template<typename T>
T& SharedPtr<T>::operator*() const {
	return *ptr;
}

template <typename T>
T* SharedPtr<T>::operator->() const {
	return ptr;
}

template <typename T>
size_t SharedPtr<T>::use_count() const {
	return ref_count ? *ref_count : 0;
}

template<typename T>
SharedPtr<T>::operator bool() const {
	return ptr;
}

template<typename T>
void SharedPtr<T>::release() {
	if (ref_count) {
		--(*ref_count);
		if (*ref_count == 0) {
			delete ptr;
			delete ref_count;
		}
	}
}
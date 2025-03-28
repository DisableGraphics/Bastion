#pragma once
#include <kernel/cpp/forward.hpp>
/**
	\brief Unique Ptr that simulates an owning pointer
 */
 template <typename T>
 class UniquePtr {
 private:
	 T* ptr;
 
 public:
	 // Default constructor
	 explicit UniquePtr(T* p = nullptr) noexcept : ptr(p) {}
 
	 // Destructor
	 ~UniquePtr() { delete ptr; }
 
	 // Delete copy constructor and copy assignment
	 UniquePtr(const UniquePtr&) = delete;
	 UniquePtr& operator=(const UniquePtr&) = delete;
 
	 // Move constructor
	 UniquePtr(UniquePtr&& other) noexcept : ptr(other.ptr) {
		 other.ptr = nullptr;
	 }
 
	 // Move assignment operator
	 UniquePtr& operator=(UniquePtr&& other) noexcept {
		 if (this != &other) {
			 delete ptr; // Clean up existing resource
			 ptr = other.ptr;
			 other.ptr = nullptr;
		 }
		 return *this;
	 }
 
	 // Overloaded dereference operator
	 T& operator*() const noexcept { return *ptr; }
 
	 // Overloaded arrow operator
	 T* operator->() const noexcept { return ptr; }
 
	 // Get the raw pointer
	 T* get() const noexcept { return ptr; }
 
	 // Release ownership of the managed object
	 T* release() noexcept {
		 T* temp = ptr;
		 ptr = nullptr;
		 return temp;
	 }
 
	 // Reset the managed object
	 void reset(T* p = nullptr) noexcept {
		 delete ptr;
		 ptr = p;
	 }
 
	 // Check if there's an associated resource
	 explicit operator bool() const noexcept { return ptr != nullptr; }
 };


template <typename T, typename... Args>
UniquePtr<T> make_unique(Args&&... args) {
	return UniquePtr<T>(new T(forward<Args>(args)...));
}
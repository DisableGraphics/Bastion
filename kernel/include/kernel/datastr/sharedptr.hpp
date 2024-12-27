#include <stddef.h>

template <typename T>
class SharedPtr {
private:
    T* ptr;                  // Raw pointer to the managed object
    size_t* ref_count;       // Reference count

    void release() {
        if (ref_count) {
            --(*ref_count);
            if (*ref_count == 0) {
                delete ptr;
                delete ref_count;
            }
        }
    }

public:
    // Default constructor
    SharedPtr() : ptr(nullptr), ref_count(nullptr) {}

    // Constructor with raw pointer
    explicit SharedPtr(T* raw_ptr) : ptr(raw_ptr), ref_count(new size_t(1)) {}

    // Copy constructor
    SharedPtr(const SharedPtr& other) : ptr(other.ptr), ref_count(other.ref_count) {
        if (ref_count) {
            ++(*ref_count);
        }
    }

    // Move constructor
    SharedPtr(SharedPtr&& other) noexcept : ptr(other.ptr), ref_count(other.ref_count) {
        other.ptr = nullptr;
        other.ref_count = nullptr;
    }

    // Copy assignment operator
    SharedPtr& operator=(const SharedPtr& other) {
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

    // Move assignment operator
    SharedPtr& operator=(SharedPtr&& other) noexcept {
        if (this != &other) {
            release();
            ptr = other.ptr;
            ref_count = other.ref_count;
            other.ptr = nullptr;
            other.ref_count = nullptr;
        }
        return *this;
    }

    // Destructor
    ~SharedPtr() {
        release();
    }

    // Accessors
    T* get() const {
        return ptr;
    }

    T& operator*() const {
        return *ptr;
    }

    T* operator->() const {
        return ptr;
    }

    size_t use_count() const {
        return ref_count ? *ref_count : 0;
    }

    explicit operator bool() const {
        return ptr != nullptr;
    }
};
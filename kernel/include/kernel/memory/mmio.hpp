#include <stdint.h>

template <typename T>
class MMIOPtr {
	public:
		MMIOPtr(uintptr_t address)
        : ptr(reinterpret_cast<volatile T*>(address)) {}
		MMIOPtr(T* address)
        : ptr(address) {}
		MMIOPtr() : ptr(nullptr){}
		
		T read() const {
			return *ptr;
		}

		void write(T value) {
			*ptr = value;
		}

		MMIOPtr& operator=(T* value) {
			ptr = value;
			return *this;
		}

		operator T() const {
			return read();
		}

		operator bool() const {
			return ptr;
		}

		T* operator->() const {
			return ptr;
		}

	private:
		volatile T* ptr;
};
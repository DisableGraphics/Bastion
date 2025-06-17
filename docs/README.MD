# README
Guidelines:
- Document everything. If someone doesn't understand what your code does, that's a nice indicator to do so.
	- Keep the documentation in the header file (so the code file doesn't become 1000+ lines long for a damn driver)

# Modules
- Kernel (codename: Ivory Tower). The kernel of the Bastion operating system
- KLibC (codename: Palisade). The Kernel's C library

# Languages
- Mandatory use of C:
	- Interfaces for C libraries
	- Implementation of C libraries and/or C interfaces that doesn't require access to kernel interfaces.
		- mem* functions, for example
	- Kernel interfaces accesible from user code.
- Mandatory use of C++:
	- Internal Kernel interfaces
	- Implementation of C libraries and/or C interfaces that requires access to kernel interfaces.
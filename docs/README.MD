# README
Guidelines:
- Document everything. If someone doesn't understand what your code does, that's a nice indicator to do so.
	- Keep the documentation in the header file (so the code file doesn't become 1000+ lines long for a damn driver)
- Try to use as least assembly as possible. Assembly is really easy to fuck up and it's usually less readable than badly written C. (Exceptions exist, and some of those live in places such as [The Daily WTF](https://thedailywtf.com/))

# Languages
- On C and C++: This codebase is made up of two different modules (as of writing):
	- Kernel: *must* be written in languages other than C. Should have as little "extern C" as possible.
		- Why: OOP helps doing kernel development (this is the author's opinion, and doesn't have to reflect yours).
		- Prefer using C++, as its OOP capabilities are much more complete than other systems programming languages.
		- Avoid C completely (unless with interfacing with assembly). The reason is simple: having a bunch of functions instead of a bunch of classes has this side effect of polluting the global namespace, and function names end up being super long. It's a good practice in the C world to prefix functions with their "namespace" (like pit_init()), but it ends up being annoying when you have to write `pagingmanager_alloc_page(&pagingmanager, virtaddr, realaddr)` instead of "just" `PagingManager::get().alloc(virtaddr, realaddr)`.
	- Kernel LibC: Interfaces *must* be C (for bloody obvious reasons), but internal code *can* be anything else.
- On Rust and Zig:
	- Rust and Zig are two languages that have begun taking off, and for a good reason. They do solve perceived or real warts on C++ and C.
	- They can be used in this codebase. However, take into account the following:
		- They are slightly better candidates for the libc than for the kernel, as the kernel uses a very hard OOP paradigm which both languages may or may not support completely. They might also have differing struct layouts in memory which aren't compatible with C++.
		- They can only work with the kernel interfaces using the libc and/or internal functions. This may reduce their perceived safety (In particular Rust's) since you have to "go back" to char *'s for text.
		- Rust has such a tight integration with cargo that it might be annoying to use it in this codebase.
		- Zig has also its build system, but it might be easier to work with.
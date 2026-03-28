# libsyscall
A group of 3 different libraries used to wrap the syscall interface into something more C-like.

They have linear dependencies between them:
- libsyswrap depends on libipc
- libipc depends on libsyscall

What they do is wrap each other: libsyscall is the lowest level (just a raw interface for syscalls), libipc less lowlevel (ipc + manually doing system calls) and finally, at the highest level, libsyswrap (which wraps all kernel functions into something more manageable and less error prone)
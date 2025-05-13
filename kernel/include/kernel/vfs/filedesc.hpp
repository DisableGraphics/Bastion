#pragma once
#include <kernel/vfs/inodes/inode.hpp>

struct FileDescriptor {
	Inode* inode;
	size_t offset = 0;
	int flags = 0;
};
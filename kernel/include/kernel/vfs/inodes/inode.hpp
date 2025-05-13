#pragma once
#include <kernel/fs/const.hpp>
#include <stddef.h>

class Inode {
	public:
		virtual off_t read(size_t offset, size_t nbytes, char* buffer) = 0;
		virtual off_t write(size_t offset, size_t nbytes, const char* buffer) = 0;
		virtual int stat(struct stat* buf) = 0;
		virtual off_t truncate(size_t newsize) = 0;
		virtual ~Inode() {}
};
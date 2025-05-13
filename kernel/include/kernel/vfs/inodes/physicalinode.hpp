#pragma once
#include <kernel/datastr/uptr.hpp>
#include <kernel/vfs/inodes/inode.hpp>
#include <kernel/fs/fs.hpp>

class PhysicalInode : public Inode {
	public:
		PhysicalInode(FS* fs, UniquePtr<char>&& path);
		~PhysicalInode();
		off_t read(size_t offset, size_t nbytes, char* buffer) override;
		off_t write(size_t offset, size_t nbytes, const char* buffer) override;
		off_t truncate(size_t newsize) override;
		int stat(struct stat* buf) override;
	private:
		FS* filesystem;
		UniquePtr<char> path;
};
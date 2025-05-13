#pragma once
#include <kernel/vfs/inodes/inode.hpp>
#include <kernel/sync/pipe.hpp>
#include <kernel/datastr/sharedptr.hpp>

class PipeInode : public Inode {
	public:
		PipeInode(const SharedPtr<Pipe> &pipeptr, bool reader);
		off_t read(size_t offset, size_t nbytes, char* buffer) override;
		off_t write(size_t offset, size_t nbytes, const char* buffer) override;
		off_t truncate(size_t newsize) override;
		int stat(struct stat* buf) override;
		~PipeInode();
	private:
		SharedPtr<Pipe> pipe;
		bool is_reader;
};
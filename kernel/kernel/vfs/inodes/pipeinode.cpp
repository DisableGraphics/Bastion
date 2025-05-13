#include <kernel/vfs/inodes/pipeinode.hpp>

PipeInode::PipeInode(const SharedPtr<Pipe> &pipeptr, bool reader) : pipe(pipeptr), is_reader(reader) {

}

PipeInode::~PipeInode() {}

off_t PipeInode::read(size_t offset, size_t nbytes, char* buffer) {
	if(is_reader)
		return pipe->read(buffer, nbytes);
	return 0;
}

off_t PipeInode::write(size_t offset, size_t nbytes, const char* buffer) {
	if(!is_reader)
		return pipe->write(buffer, nbytes);
	return 0;
}

off_t PipeInode::truncate(size_t) {
	return -1;
}

int PipeInode::stat(struct stat *buf) {
	buf->st_atime = 0;
	buf->st_ctime = 0;
	buf->st_ino = 0;
	buf->st_mtime = 0;
	buf->st_size = 0;
	return 0;
}
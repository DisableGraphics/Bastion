#include <kernel/cpp/move.hpp>
#include <kernel/vfs/inodes/physicalinode.hpp>

PhysicalInode::PhysicalInode(FS* filesystem, UniquePtr<const char>&& path) : filesystem(filesystem), path(move(path)) {

}

PhysicalInode::~PhysicalInode() {
}

off_t PhysicalInode::read(size_t offset, size_t nbytes, char* buffer) {
	return filesystem->read(path.get(), offset, nbytes, buffer);
}

off_t PhysicalInode::write(size_t offset, size_t nbytes, const char* buffer) {
	return filesystem->write(path.get(), offset, nbytes, buffer);
}

off_t PhysicalInode::truncate(size_t newsize) {
	return filesystem->truncate(path.get(), newsize);
}

int PhysicalInode::stat(struct stat* buf) {
	return filesystem->stat(path.get(), buf);
}
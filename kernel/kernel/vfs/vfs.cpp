#include "kernel/vfs/inodes/physicalinode.hpp"
#include <kernel/vfs/vfs.hpp>

VFS& VFS::get() {
	static VFS instance;
	return instance;
}

VFS::VFS() {

}

int VFS::open(const char* path, int flags) {
	Inode* inode = resolvePath(path);
	if (!inode) return -1;

	/// TODO: handle flags
	int fd = next_fd++;
	fd_table.insert(fd, FileDescriptor{ inode, 0, flags });
	return fd;
}

off_t VFS::read(int fd, void* buf, size_t count) {
	FileDescriptor* ino = fd_table.find(fd);
	if(!ino) return -1;
	off_t bytes = ino->inode->read(ino->offset, count, static_cast<char*>(buf));
	if(bytes > 0) ino->offset += bytes;
	return bytes;
}

off_t VFS::write(int fd, const void* buf, size_t count) {
	auto ino = fd_table.find(fd);
	if (!ino) return -1;
	ssize_t bytes = ino->inode->write(ino->offset, count, static_cast<const char*>(buf));
	if (bytes > 0) ino->offset += bytes;
	return bytes;
}

int VFS::stat(int fd, struct stat* buffer) {
	if(!buffer) return -1;
	auto ino = fd_table.find(fd);
	if (!ino) return -1;
	return ino->inode->stat(buffer);
}

int VFS::close(int fd) {
	return fd_table.erase(fd) ? 0 : -1;
}

Inode* VFS::resolvePath(const char* path) {
	MountPoint* best = findMountPoint(path);
	if(!best) return nullptr;
	const size_t best_path_size = strlen(best->path);
	const char* internalPath = path + best_path_size;
	if(strlen(internalPath) == 0) internalPath = const_cast<char*>("/");
	
	const size_t size = best_path_size + 2 + strlen(internalPath);
	char* cache_key = reinterpret_cast<char*>(kcalloc(size, sizeof(char)));
	memcpy(cache_key, best->path, best_path_size);
	memcpy(cache_key + best_path_size, ":", 1);
	memcpy(cache_key + 1 + best_path_size, internalPath, strlen(internalPath));

	PhysicalInode* inode = new PhysicalInode(best->fs, UniquePtr<const char>(internalPath));
	inode_cache.insert(cache_key, inode);
	return inode;
}

int VFS::mount(const char* path, FS* fs) {
	for(size_t i = 0; i < mounts.size(); i++) {
		if(!strcmp(mounts[i].path, path)) { // Already mounted
			return -1;
		}
	}
	mounts.push_back(MountPoint{path, fs});
	return 0;
}

int VFS::umount(const char* path) {
	for(size_t i = 0; i < mounts.size(); i++) {
		if(!strcmp(mounts[i].path, path)) {
			mounts.erase(i);
			return 0;
		}
	}
	return -1;
}

MountPoint* VFS::findMountPoint(const char* path) {
	MountPoint* ret = nullptr;
	size_t retlen = 0;
	for(size_t i = 0; i < mounts.size(); i++) {
		const size_t mountlen = strlen(mounts[i].path);
		if(!memcmp(mounts[i].path, path, mountlen)) { // Already mounted
			if(!ret || mountlen > retlen) {
				ret = &mounts[i];
				retlen = mountlen;
			}
		}
	}
	return ret;
}

int VFS::truncate(int fd, size_t newsize) {
	auto ino = fd_table.find(fd);
	if (!ino) return -1;
	return ino->inode->truncate(newsize);
}
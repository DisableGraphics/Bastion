#include "kernel/vfs/inodes/physicalinode.hpp"
#include <kernel/vfs/vfs.hpp>
#include <kernel/kernel/log.hpp>

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
	MountPoint * best;
	const char* internalPath;
	if(!getMountPointAndInternalPath(path, &best, &internalPath)) return nullptr;
	const size_t best_path_size = strlen(best->path);
	
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
	bool last_slash = true;
	size_t size = strlen(path);
	if(path[size - 1] != '/') {
		size++;
		last_slash = false;
	}
	char* mallc = (char*)kmalloc(size+1);
	if(!last_slash) {
		strncpy(mallc, path, size-1);
		mallc[size-1] = '/';
	} else {
		strncpy(mallc, path, size);
	}
	mallc[size] = 0;
	mounts.push_back(MountPoint{mallc, fs});
	return 0;
}

int VFS::umount(const char* path) {
	for(size_t i = 0; i < mounts.size(); i++) {
		if(!strcmp(mounts[i].path, path)) {
			kfree((void*)mounts[i].path);
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

bool VFS::touch(const char* filename) {
	MountPoint * best;
	const char* internalPath;
	if(!getMountPointAndInternalPath(filename, &best, &internalPath)) return false;
	return best->fs->touch(internalPath);
}

bool VFS::mkdir(const char* dirname) {
	MountPoint * best;
	const char* internalPath;
	if(!getMountPointAndInternalPath(dirname, &best, &internalPath)) return false;
	return best->fs->mkdir(internalPath);
}

bool VFS::rmdir(const char* dirname) {
	MountPoint * best;
	const char* internalPath;
	if(!getMountPointAndInternalPath(dirname, &best, &internalPath)) return false;
	return best->fs->rmdir(internalPath);
}

bool VFS::rename(const char* origin, const char* destination) {
	MountPoint * best, best2;
	const char* internalPath;
	if(!getMountPointAndInternalPath(origin, &best, &internalPath)) return false;
	if(!getMountPointAndInternalPath(destination, &best, &internalPath)) return false;
	if(strcmp(best->path, best2.path)) return false;
	return best->fs->rename(origin, destination);
}

bool VFS::remove(const char* filename) {
	MountPoint * best;
	const char* internalPath;
	if(!getMountPointAndInternalPath(filename, &best, &internalPath)) return false;
	return best->fs->remove(internalPath);
}

bool VFS::opendir(const char* directory, DIR* dir) {
	log(INFO, "Opening directory: %s", directory);
	MountPoint * best;
	const char* internalPath;
	if(!getMountPointAndInternalPath(directory, &best, &internalPath)) return false;
	dir->filesystem = best->fs;
	return best->fs->opendir(internalPath, dir);
}

bool VFS::readdir(DIR* dir, dirent* dirent) {
	FS* fs = reinterpret_cast<FS*>(dir->filesystem);
	return fs->readdir(dir, dirent);
}

void VFS::closedir(DIR* dir) {
	FS* fs = reinterpret_cast<FS*>(dir->filesystem);
	return fs->closedir(dir);
}

bool VFS::getMountPointAndInternalPath(const char* path, MountPoint** mnt, const char** internal) {
	MountPoint* best = findMountPoint(path);
	if(!best) return false;
	const size_t best_path_size = strlen(best->path);
	const char* internalPath = path + best_path_size - 1;
	if(strlen(internalPath) == 0) internalPath = const_cast<char*>("/");
	*mnt = best;
	*internal = internalPath;
	log(INFO, "%s has a mountpoint %s and internal path %s", path, best->path, internalPath);
	return true;
}
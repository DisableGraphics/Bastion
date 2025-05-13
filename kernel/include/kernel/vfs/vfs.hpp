#pragma once
#include <kernel/fs/fs.hpp>
#include <kernel/vfs/inodes/inode.hpp>
#include <kernel/datastr/vector.hpp>
#include <kernel/vfs/mountpoint.hpp>
#include <kernel/datastr/hashmap/hashmap.hpp>
#include <kernel/vfs/filedesc.hpp>

class VFS {
	public:
		static VFS& get();
		int open(const char* path, int flags);
		off_t read(int fd, void* buf, size_t count);
		off_t write(int fd, const void* buf, size_t count);
		int stat(int fd, struct stat* buf);
		int truncate(int fd, size_t newsize);
		int close(int fd);

		int mount(const char* path, FS* fs);
	private:
		VFS();
		VFS(const VFS&) = delete;
		VFS(VFS&&) = delete;
		VFS& operator=(const VFS&) = delete;
		VFS& operator=(VFS&&) = delete;

		Inode* resolvePath(char* path);

		Vector<MountPoint> mounts;
		HashMap<char*, Inode*> inode_cache;
		HashMap<int, FileDescriptor> fd_table;

		MountPoint* findMountPoint(const char* path);
		int next_fd = 0;
};
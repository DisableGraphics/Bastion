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
		// File operations
		int open(const char* path, int flags);
		off_t read(int fd, void* buf, size_t count);
		off_t write(int fd, const void* buf, size_t count);
		int stat(int fd, struct stat* buf);
		int truncate(int fd, size_t newsize);
		int close(int fd);

		// Directory operations
		bool touch(const char* filename);
		bool mkdir(const char* directory_name);
		bool rmdir(const char* directory_name);
		bool rename(const char* src, const char* dest);
		bool remove(const char* path);
		bool opendir(const char* directory, DIR* dir);
		bool readdir(DIR* dir, dirent* dirent);
		void closedir(DIR* dir);

		// Mount operations
		int mount(const char* path, FS* fs);
		int umount(const char* path);
	private:
		VFS();
		VFS(const VFS&) = delete;
		VFS(VFS&&) = delete;
		VFS& operator=(const VFS&) = delete;
		VFS& operator=(VFS&&) = delete;

		Inode* resolvePath(const char* path);

		bool getMountPointAndInternalPath(const char* path, MountPoint** mnt, const char** internal);

		Vector<MountPoint> mounts;
		HashMap<char*, Inode*> inode_cache;
		HashMap<int, FileDescriptor> fd_table;

		MountPoint* findMountPoint(const char* path);
		int next_fd = 0;
};
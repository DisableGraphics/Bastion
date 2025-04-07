#pragma once
#include "const.hpp"

class FS {
	public:
		/**
			\brief Read a file from the filesystem.
			\param filename The name of the file to read.
			\param offset The offset in the file to start reading from.
			\param nbytes The number of bytes to read.
			\param buffer The buffer to read the data into.
			\return The number of bytes read, or -1 on error.
		*/
		virtual off_t read(const char* filename, unsigned offset, unsigned nbytes, char* buffer) = 0;
		/**
			\brief Write to a file in the filesystem.
			\param filename The name of the file to write to.
			\param offset The offset in the file to start writing to.
			\param nbytes The number of bytes to write.
			\param buffer The buffer containing the data to write.
			\return The number of bytes written, or -1 on error.
		*/
		virtual off_t write(const char* filename, unsigned offset, unsigned nbytes, const char* buffer) = 0;
		/**
			\brief Truncate a file in the filesystem.
			\param filename The name of the file to truncate.
			\param nbytes The new size of the file.
			\return The new size of the file, or -1 on error.
		*/
		virtual off_t truncate(const char* filename, unsigned nbytes) = 0;
		/**
			\brief Get the status of a file in the filesystem.
			\param filename The name of the file to get the status of.
			\param buf A pointer to a stat structure to fill with the file's status.
			\return 0 on success, -1 on error.
		*/
		virtual int stat(const char* filename, struct stat* buf) = 0;
		/**
			\brief Create a new file in the filesystem.
			\param filename The name of the file to create.
			\return true on success, false on error.
		*/
		virtual bool touch(const char* filename) = 0;
		/**
			\brief Create a new directory in the filesystem.
			\param directory The name of the directory to create.
			\return true on success, false on error.
		*/
		virtual bool mkdir(const char* directory) = 0;
		/**
			\brief Remove a directory from the filesystem.
			\param directory The name of the directory to remove.
			\return true on success, false on error.
		*/
		virtual bool rmdir(const char* directory) = 0;
		/**
			\brief Rename a file or directory in the filesystem.
			\param src The current name of the file or directory.
			\param dest The new name of the file or directory.
			\return true on success, false on error.
		*/
		virtual bool rename(const char* src, const char* dest) = 0;
		/**
			\brief Remove a file from the filesystem.
			\param path The name of the file to remove.
			\return true on success, false on error.
		*/
		virtual bool remove(const char* path) = 0;
		/**
			\brief Open a directory in the filesystem.
			\param directory The name of the directory to open.
			\param dir A pointer to a DIR structure to fill with the directory's information.
			\return true on success, false on error.
		*/
		virtual bool opendir(const char* directory, DIR* dir) = 0;
		/**
			\brief Read a directory entry from the filesystem.
			\param dir A pointer to a DIR structure containing the directory's information.
			\param dirent A pointer to a dirent structure to fill with the directory entry's information.
			\return true on success, false on error.
		*/
		virtual bool readdir(DIR* dir, dirent* dirent) = 0;
		/**
			\brief Close a directory in the filesystem.
			\param dir A pointer to a DIR structure containing the directory's information.
		*/
		virtual void closedir(DIR* dir) = 0;
};
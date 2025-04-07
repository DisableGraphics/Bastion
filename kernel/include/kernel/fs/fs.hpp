#pragma once
#include "const.hpp"

class FS {
	public:
		virtual off_t read(const char* filename, unsigned offset, unsigned nbytes, char* buffer) = 0;
		virtual off_t write(const char* filename, unsigned offset, unsigned nbytes, const char* buffer) = 0;
		virtual off_t truncate(const char* filename, unsigned nbytes) = 0;
		virtual int stat(const char* filename, struct stat* buf) = 0;

		virtual bool touch(const char* filename) = 0;

		virtual bool mkdir(const char* directory) = 0;
		virtual bool rmdir(const char* directory) = 0;
		
		virtual bool rename(const char* src, const char* dest) = 0;
		virtual bool remove(const char* path) = 0;

		virtual bool opendir(const char* directory, DIR* dir) = 0;
		virtual bool readdir(DIR* dir, dirent* dirent) = 0;
		virtual void closedir(DIR* dir) = 0;
};
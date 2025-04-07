#pragma once
#include "fs.hpp"

// Ramdisk USTAR filesystem
// Only for getting data at boot.
class RAMUSTAR : public FS {
	public:
		RAMUSTAR(const uint8_t* where) : where(where) {};
		~RAMUSTAR() = default;

		off_t read(const char* filename, unsigned offset, unsigned nbytes, char* buffer) override;
		off_t write(const char* filename, unsigned offset, unsigned nbytes, const char* buffer) override;
		off_t truncate(const char* filename, unsigned nbytes) override;
		int stat(const char* filename, struct stat* buf) override;

		bool touch(const char* filename) override;

		bool mkdir(const char* directory) override;
		bool rmdir(const char* directory) override;

		bool rename(const char* src, const char* dest) override;
		bool remove(const char* path) override;

		bool opendir(const char* directory, DIR* dir) override;
		bool readdir(DIR* dir, dirent* dirent) override;
		void closedir(DIR* dir) override;
	private:
		const uint8_t* where;
};
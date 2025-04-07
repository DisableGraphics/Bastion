#include <kernel/fs/ramustar.hpp>
#include <string.h>

int oct2bin(char *str, int size) {
    int n = 0;
    char *c = str;
    while (size-- > 0) {
        n *= 8;
        n += *c - '0';
        c++;
    }
    return n;
}

int RAMUSTAR::lookup(char *archive, char *filename, char **out) {
	char *ptr = archive;

    while (!memcmp(ptr + 257, "ustar", 5)) {
        int filesize = oct2bin(ptr + 0x7c, 11);
        if (!memcmp(ptr, filename, strlen(filename) + 1)) {
            *out = ptr + 512;
            return filesize;
        }
        ptr += (((filesize + 511) / 512) + 1) * 512;
    }
    return 0;

}

off_t RAMUSTAR::read(const char* filename, unsigned offset, unsigned nbytes, char* buffer) {
	char *out;
	int size = lookup((char*)where, (char*)filename, &out);
	if (size == 0) {
		return -1;
	}
	if (offset >= size) {
		return 0;
	}
	if (offset + nbytes > size) {
		nbytes = size - offset;
	}
	memcpy(buffer, out + offset, nbytes);
	return nbytes;
}

off_t RAMUSTAR::write(const char* filename, unsigned offset, unsigned nbytes, const char* buffer) {
	return -1;
}
off_t RAMUSTAR::truncate(const char* filename, unsigned nbytes) {
	return -1;
}

int RAMUSTAR::stat(const char* filename, struct stat* buf) {
	char *out;
	int size = lookup((char*)where, (char*)filename, &out);
	if (size == 0) {
		return -1;
	}
	buf->st_size = size;
	buf->st_ino = 0;
	buf->st_ctime = 0;
	buf->st_atime = 0;
	buf->st_mtime = 0;
	return 0;
}

bool RAMUSTAR::touch(const char* filename) {
	return false;
}

bool RAMUSTAR::mkdir(const char* directory) {
	return false;
}

bool RAMUSTAR::rmdir(const char* directory) {
	return false;
}

bool RAMUSTAR::rename(const char* src, const char* dest) {
	return false;
}

bool RAMUSTAR::remove(const char* path) {
	return false;
}

bool RAMUSTAR::opendir(const char* directory, DIR* dir) {
	return false;
}

bool RAMUSTAR::readdir(DIR* dir, dirent* dirent) {
	return false;
}

void RAMUSTAR::closedir(DIR* dir) {
}
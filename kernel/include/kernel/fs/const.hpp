#pragma once
#include <stdint.h>
#include <time.h>
typedef int64_t off_t;
typedef uint32_t ino_t;

struct stat {
	off_t st_size;

	time_t st_atime; // Last access
	time_t st_mtime; // Last modification
	time_t st_ctime; // Creation time

	ino_t st_ino;
};
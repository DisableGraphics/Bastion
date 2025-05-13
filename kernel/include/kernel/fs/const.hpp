#pragma once
#include <stdint.h>
#include <time.h>
typedef int64_t off_t;
typedef int32_t ssize_t;
typedef uint32_t ino_t;

enum DTTYPE : unsigned char {
	DT_BLK,
	DT_CHR,
	DT_DIR,
	DT_FIFO,
	DT_LNK,
	DT_REG,
	DT_SOCK,
	DT_UNKNOWN
};

struct stat {
	off_t st_size;

	time_t st_atime; // Last access
	time_t st_mtime; // Last modification
	time_t st_ctime; // Creation time

	ino_t st_ino;
};

struct DIR {
	/// First cluster
	ino_t d_ino;
	/// Current cluster
	ino_t d_curino;
	/// Current entry
	uint32_t d_curentry;
};

struct dirent {
	ino_t d_ino;
	unsigned char d_type;
	char d_name[256];
};
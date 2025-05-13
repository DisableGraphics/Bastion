#pragma once
#include <kernel/fs/fs.hpp>

struct MountPoint {
	const char* path;
	FS* fs;
};
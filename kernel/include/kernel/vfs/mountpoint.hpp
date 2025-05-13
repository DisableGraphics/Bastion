#pragma once
#include <kernel/fs/fs.hpp>

struct MountPoint {
	char* path;
	FS* fs;
};
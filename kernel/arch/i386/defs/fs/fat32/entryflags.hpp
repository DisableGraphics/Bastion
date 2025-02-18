#pragma once
enum class FAT_FLAGS {
	NONE = 0,
	READ_ONLY = 0x1,
	HIDDEN = 0x2,
	SYSTEM = 0x4,
	VOLUME_ID = 0x8,
	DIRECTORY = 0x10,
	ARCHIVE = 0x20,
	LFN = 0xf,
	ALL= 0xff
};
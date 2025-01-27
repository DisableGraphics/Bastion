#pragma once

/**
	\brief Struct that holds the name of a disk
 */
struct diskname {
	diskname(const char *type, char number);
	diskname(const diskname&);
	diskname(diskname&&);
	diskname& operator=(const diskname&);
	diskname& operator=(diskname&&);
	/// Automatic conversion to string
	operator char*();
	/// Type of disk (e.g 'ahci')
	char type[4];
	/// Number of disk
	char number;
	/// Null terminator for string conversion
	const char zero = '\0';
};

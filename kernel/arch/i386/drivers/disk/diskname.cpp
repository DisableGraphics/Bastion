#include <kernel/drivers/disk/diskname.hpp>
#include <string.h>

diskname::diskname(const char *type, char number) {
	memcpy(this->type, type, sizeof(this->type));
	this->number = number;
}

diskname::diskname(const diskname& other) {
	memcpy(this, &other, sizeof(diskname));
}

diskname::diskname(diskname&& other) {
	memcpy(this, &other, sizeof(diskname));
}

diskname& diskname::operator=(const diskname& other) {
	memcpy(this, &other, sizeof(diskname));
	return *this;
}

diskname& diskname::operator=(diskname&& other) {
	memcpy(this, &other, sizeof(diskname));
	return *this;
}

diskname::operator char *() {
	return reinterpret_cast<char*>(this);
}
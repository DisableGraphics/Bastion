#pragma once

#include <stddef.h>
const size_t PAT = 0x0800;
const size_t GLOBAL = 0x100;
const size_t LONG_PAGE_BIT = 0x80;
const size_t DIRTY = 0x40;
const size_t ACCESSED = 0x20;
const size_t CACHE_DISABLE = 0x10;
const size_t WRITE_THROUGH = 0x08;
const size_t USER = 0x04;
const size_t READ_WRITE = 0x02;
const size_t PRESENT = 0x01;

const size_t PAGE_SIZE = 4096;
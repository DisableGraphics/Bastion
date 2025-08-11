#include <liballoc/liballoc.h>
#include <kernel/memory/mmanager.hpp>
#include <kernel/sync/spinlock.hpp>

SpinLock mtx;

extern "C" int liballoc_lock() {
	mtx.lock();
	return 0;
}

extern "C" int liballoc_unlock() {
	mtx.unlock();
	return 0;
}

extern "C" void *liballoc_alloc(size_t pages) {
	return MemoryManager::get().alloc_pages_debug(pages, READ_WRITE);
}

extern "C" int liballoc_free(void* start, size_t pages) {
	MemoryManager::get().free_pages_debug(start, pages);
	return 0;
}

void *operator new(size_t size)
{
	return kmalloc(size);
}

void *operator new[](size_t size)
{
	return kmalloc(size);
}

void operator delete(void *p) throw()
{
	kfree(p);
}

void operator delete[](void *p) throw()
{
	kfree(p);
}

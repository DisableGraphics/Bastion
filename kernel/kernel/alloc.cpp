#include <liballoc/liballoc.h>
#include <kernel/mmanager.hpp>
#include <kernel/mutex.hpp>

Mutex mtx;

int liballoc_lock() {
	mtx.lock();
	return 0;
}

int liballoc_unlock() {
	mtx.unlock();
	return 0;
}

void *liballoc_alloc(size_t pages) {
	return MemoryManager::get().alloc_pages(pages);
}

int liballoc_free(void* start, size_t pages) {
	MemoryManager::get().free_pages(start, pages);
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
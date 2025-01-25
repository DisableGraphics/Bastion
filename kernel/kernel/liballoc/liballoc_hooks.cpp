#include <liballoc/liballoc.h>
#include <kernel/memory/mmanager.hpp>
#include <kernel/sync/mutex.hpp>

SpinMutex mtx;

int liballoc_lock() {
	mtx.lock();
	return 0;
}

int liballoc_unlock() {
	mtx.unlock();
	return 0;
}

void *liballoc_alloc(int pages) {
	return MemoryManager::get().alloc_pages(pages);
}

int liballoc_free(void* start, int pages) {
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
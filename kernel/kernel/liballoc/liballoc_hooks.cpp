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

extern "C" void *liballoc_alloc(int pages) {
	return MemoryManager::get().alloc_pages_debug(pages, READ_WRITE);
}

extern "C" int liballoc_free(void* start, int pages) {
	MemoryManager::get().free_pages_debug(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(start) - HIGHER_HALF_OFFSET), pages);
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
#include <stdint.h>
constexpr uint32_t MUTEX_UNLOCK = 0;
constexpr uint32_t MUTEX_LOCK = 1;

class Mutex {
	public:
		Mutex() : lock_var(MUTEX_UNLOCK) {}
		void lock();
		void unlock();
	private:
    	volatile uint32_t lock_var;

};

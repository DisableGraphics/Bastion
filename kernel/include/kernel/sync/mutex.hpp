#include <stdint.h>
constexpr uint32_t MUTEX_UNLOCK = 0;
constexpr uint32_t MUTEX_LOCK = 1;

/**
	\brief Mutex syncronisation primitive
 */
class Mutex {
	public:
		/**
			\brief Constructor. Mutex is unlocked
		 */
		Mutex() : lock_var(MUTEX_UNLOCK) {}
		/**
			\brief Lock mutex.
		 */
		void lock();
		/**
			\brief Unlock mutex.
		 */
		void unlock();
	private:
    	volatile uint32_t lock_var;

};

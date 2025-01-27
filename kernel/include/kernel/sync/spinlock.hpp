#include <stdint.h>
constexpr uint32_t SPINLOCK_UNLOCK = 0;
constexpr uint32_t SPINLOCK_LOCK = 1;

/**
	\brief Spinlock.
	\details Vinny would be proud.
 */
class SpinLock {
	public:
		/**
			\brief Constructor. Spinlock is unlocked
		 */
		SpinLock() : lock_var(SPINLOCK_UNLOCK) {}
		/**
			\brief Lock spinlock.
		 */
		void lock();
		/**
			\brief Unlock spinlock.
		 */
		void unlock();
	private:
		volatile uint32_t lock_var;

};

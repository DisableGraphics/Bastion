#include <kernel/sync/semaphore.h>
/**
	\brief Semaphore syncronisation primitive
 */
class SpinSemaphore {
	public:
		/**
			\brief Initialise semaphore to count.
		 */
		SpinSemaphore(int count);
		/**
			\brief Destructor for the semaphore.
			Frees resources automatically.
		 */
		~SpinSemaphore();
		/**
			\brief Await for semaphore.
		 */
		void await();
		/**
			\brief Signal the semaphore.
		 */
		void signal();
	private:
		// Internal semaphore
		spin_sem_t sem;
};
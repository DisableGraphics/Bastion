#include <kernel/sync/semaphore.h>
/**
	\brief Semaphore syncronisation primitive
 */
class Semaphore {
	public:
		/**
			\brief Initialise semaphore to count.
		 */
		Semaphore(int count);
		/**
			\brief Destructor for the semaphore.
			Frees resources automatically.
		 */
		~Semaphore();
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
		sem_t sem;
};
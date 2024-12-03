#include <kernel/sync/semaphore.h>

class Semaphore {
	public:
		Semaphore(int count);
		~Semaphore();
		void await();
		void signal();
	private:
		sem_t sem;
};
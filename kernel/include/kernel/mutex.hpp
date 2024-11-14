#include <stdatomic.h>

class ExplicitMutex {
	public:
		void acquire();
		void release();
	protected:
		atomic_flag mutex;
};

class Mutex {
	public:
		Mutex();
		~Mutex();
	private:
		ExplicitMutex mut;
};
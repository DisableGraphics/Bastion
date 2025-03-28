#include <kernel/scheduler/task.hpp>
#include <kernel/datastr/vector.hpp>

class RWLock {
	public:
		RWLock();
		void read();
		void write();

		void unlockread();
		void unlockwrite();
	private:
		void unblock();
		int nreaders = 0;
		int nwriters = 0;
		Vector<Task*> waiting_readers;
		Vector<Task*> waiting_writers;
};
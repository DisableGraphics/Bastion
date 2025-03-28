#include <kernel/sync/rwlock.hpp>
#include <kernel/scheduler/scheduler.hpp>
#include <kernel/drivers/interrupts.hpp>

RWLock::RWLock() {

}

void RWLock::read() {
	IDT::disable_interrupts();
	if(nwriters > 0) {
		waiting_readers.push_back(Scheduler::get().get_current_task());
		Scheduler::get().block(TaskState::WAITING);
	} else {
		nreaders++;
	}
}

void RWLock::write() {
	IDT::disable_interrupts();
	if(nreaders > 0 || nwriters > 0) {
		waiting_writers.push_back(Scheduler::get().get_current_task());
		Scheduler::get().block(TaskState::WAITING);
	} else {
		nwriters++;
	}
}

void RWLock::unblock() {
	// Give priority to writers
	if(!waiting_writers.empty()) {
		Task* task = waiting_writers[0];
		waiting_writers.erase(0);
		nwriters++;
		Scheduler::get().unblock(task);
	} else while(!waiting_readers.empty()) { // Unblock all readers (yes that's a while loop)
		Task* task = waiting_readers.back();
		waiting_readers.pop_back();
		nreaders++;
		Scheduler::get().unblock(task);
	}
}

void RWLock::unlockread() {
	IDT::disable_interrupts();
	nreaders--;
	if(nreaders == 0) {
		unblock();
	}
}

void RWLock::unlockwrite() {
	IDT::disable_interrupts();
	nwriters--;
	if(nwriters == 0) {
		unblock();
	}
}
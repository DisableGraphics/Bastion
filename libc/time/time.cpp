#include <time.h>
#include <kernel/time/time.hpp>

time_t time(time_t* tloc) {
	time_t currtime = TimeManager::get().get_time();
	if(tloc)
		*tloc = currtime;
	return currtime;
}
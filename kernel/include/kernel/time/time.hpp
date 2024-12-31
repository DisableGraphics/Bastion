#include <stdint.h>
#include <time.h>

class TimeManager {
	public:
		static TimeManager &get();
		void set_time(time_t);
		time_t get_time();

		static inline bool is_leap_year(uint32_t year);
		static uint32_t days_until_year(uint32_t year);
		static uint32_t days_in_year_until_month_day(uint32_t year, uint8_t month, uint8_t day);
	private:
		time_t current_time = 0;
		TimeManager(){}
};
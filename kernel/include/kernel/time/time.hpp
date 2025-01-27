#include <stdint.h>
#include <time.h>
/**
	\brief Keeps track of time. 
	\details Time is tracked using UNIX time (Seconds since 00:00:00 01/01/1970).
 */
class TimeManager {
	public:
		/**
			\brief Get singleton instance
		 */
		static TimeManager &get();
		/**
			\brief Sets current time
			\param time current UNIX time
		 */
		void set_time(time_t time);
		/**
			\brief Get current time in UNIX time
		 */
		time_t get_time();
		/**
			\brief Whether year is a leap year.
			\param year Year
		 */
		static inline bool is_leap_year(uint32_t year);
		/**
			\brief Days since the UNIX epoch (1/1/1970) to year
			\param year The end year of the counting
		 */
		static uint32_t days_until_year(uint32_t year);
		/**
			\brief Get days in a year until the day day of the month month
			\param year Year
			\param month Month
			\param day day
		 */
		static uint32_t days_in_year_until_month_day(uint32_t year, uint8_t month, uint8_t day);
	private:
		time_t current_time = 0;
		TimeManager(){}
};
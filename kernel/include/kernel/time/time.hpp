#pragma once
#include <stdint.h>
#include <time.h>

struct date {
	uint8_t hour;
	uint8_t minute;
	uint8_t second;

	uint16_t year;
	uint8_t month;
	uint8_t day;
};

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
			\brief Advance one second the internal timer
		 */
		void next_second();
		/**
			\brief Get seconds since the OS has booted
		 */
		time_t get_seconds_since_boot();
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

		static time_t date_to_timestamp(const date &date);
		static date timestamp_to_date(time_t timestamp);
	private:
		time_t current_time = 0;
		time_t seconds_since_boot = 0;
		TimeManager(){}
};
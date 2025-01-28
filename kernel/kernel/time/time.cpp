#include <kernel/time/time.hpp>

TimeManager& TimeManager::get() {
	static TimeManager instance;
	return instance;
}

void TimeManager::set_time(int64_t timestamp) {
	current_time = timestamp;
}

int64_t TimeManager::get_time() {
	return current_time;
}

bool TimeManager::is_leap_year(uint32_t year) {
	return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

uint32_t TimeManager::days_until_year(uint32_t year) {
    uint32_t days = 0;
    for (uint32_t y = 1970; y < year; ++y) {
        days += 365 + is_leap_year(y);
    }
    return days;
}

uint32_t TimeManager::days_in_year_until_month_day(uint32_t year, uint8_t month, uint8_t day) {
	static const uint16_t days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    uint32_t days = 0;
    for (uint8_t m = 0; m < month - 1; ++m) {
        days += days_in_month[m];
        if (m == 1 && is_leap_year(year)) { // February in a leap year
            days += 1;
        }
    }
    days += day - 1; // Days are 1-based
    return days;
}

void TimeManager::next_second() {
	seconds_since_boot++;
}

time_t TimeManager::get_seconds_since_boot() {
	return seconds_since_boot;
}
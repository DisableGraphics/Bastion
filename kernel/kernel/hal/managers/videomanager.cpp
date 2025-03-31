#include <kernel/hal/managers/videomanager.hpp>

hal::VideoManager& hal::VideoManager::get() {
	static hal::VideoManager instance;
	return instance;
}

size_t hal::VideoManager::register_driver(hal::VideoDriver* driver) {
	drivers.push_back(driver);
	return drivers.size() - 1;
}

size_t hal::VideoManager::get_first_screen() {
	return drivers.empty() ? -1 : 0;
}
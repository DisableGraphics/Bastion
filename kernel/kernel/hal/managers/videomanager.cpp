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

void hal::VideoManager::draw_char(size_t screen, char c, int x, int y) {
	drivers[screen]->draw_char(c, x, y);
}
void hal::VideoManager::draw_string(size_t screen, char* str, int x, int y) {
	drivers[screen]->draw_string(str, x, y);
}
void hal::VideoManager::draw_pixel(size_t screen, int x, int y, color c) {
	drivers[screen]->draw_pixel(x, y, c);
}
void hal::VideoManager::draw_rectangle(size_t screen, int x, int y, int w, int h, color c) {
	drivers[screen]->draw_rectangle(x, y, w, h, c);
}
void hal::VideoManager::clear(size_t screen) {
	drivers[screen]->clear();
}

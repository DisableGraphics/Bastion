#pragma once
#include <kernel/datastr/vector.hpp>
#include <kernel/hal/drvbase/video.hpp>

namespace hal {
	class VideoManager final {
		public:
			static VideoManager& get();
			size_t register_driver(VideoDriver* driver);
			size_t get_first_screen();
			inline void draw_char(size_t screen, unsigned c, int x, int y) {
				drivers[screen]->draw_char(c, x, y);
			}
			inline void draw_pixel(size_t screen, int x, int y, color c) {
				drivers[screen]->draw_pixel(x, y, c);
			}
			inline void draw_pixels(size_t screen, int x1, int y1, int w, int h, uint8_t* pixels) {
				drivers[screen]->draw_pixels(x1, y1, w, h, pixels);
			}
			inline void draw_rectangle(size_t screen, int x1, int y1, int x2, int y2, color c) {
				drivers[screen]->draw_rectangle(x1, y1, x2, y2, c);
			}
			inline void clear(size_t screen, color c) {
				drivers[screen]->clear(c);
			}
			inline void flush(size_t screen) {
				drivers[screen]->flush();
			};
			inline VideoDriver* get_driver(size_t screen) {
				return drivers[screen];
			}

			uint8_t get_font_width(size_t screen) {
				return drivers[screen]->get_font_width();
			}
			uint8_t get_font_height(size_t screen) {
				return drivers[screen]->get_font_height();
			}
			uint8_t get_width(size_t screen) {
				return drivers[screen]->get_width();
			}
			uint8_t get_height(size_t screen) {
				return drivers[screen]->get_height();
			}
		private:
			VideoManager(){};
			VideoManager(const VideoManager&) = delete;
			VideoManager(VideoManager&&) = delete;
			void operator=(const VideoManager&) = delete;
			void operator=(VideoManager&&) = delete;
			Vector<VideoDriver*> drivers;
	};
}
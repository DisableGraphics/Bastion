#pragma once
#include <kernel/datastr/vector.hpp>
#include <kernel/hal/drvbase/video.hpp>

namespace hal {
	class VideoManager final {
		public:
			static VideoManager& get();
			size_t register_driver(VideoDriver* driver);
			size_t get_first_screen();
			void draw_char(size_t screen, char c, int x, int y);
			void draw_string(size_t screen, char* str, int x, int y);
			void draw_pixel(size_t screen, int x, int y, color c);
			void draw_rectangle(size_t screen, int x, int y, int w, int h, color c);
			void clear(size_t screen);
			void flush(size_t screen);

		private:
			VideoManager(){};
			VideoManager(const VideoManager&) = delete;
			VideoManager(VideoManager&&) = delete;
			void operator=(const VideoManager&) = delete;
			void operator=(VideoManager&&) = delete;
			Vector<VideoDriver*> drivers;
	};
}
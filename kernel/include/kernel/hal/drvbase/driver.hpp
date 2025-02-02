#pragma once
#include <stddef.h>
#include <kernel/hal/device.hpp>

namespace hal {
	class Driver {
		public:
			~Driver() = default;
			virtual void init() = 0;
			virtual void handle_interrupt() = 0;

			virtual size_t get_irqline();
			virtual bool is_exclusive_irq();

			virtual Device get_device_type();

			virtual void basic_setup(hal::Device);
		protected:
			size_t irqline = -1;
			bool exclusive_irq = false;
			Device device_type;
	};
}
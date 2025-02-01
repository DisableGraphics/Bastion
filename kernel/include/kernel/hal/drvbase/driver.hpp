#pragma once
#include <stddef.h>

namespace hal {
	class Driver {
		public:
			~Driver() = default;
			virtual void init() = 0;
			virtual void handle_interrupt() = 0;

			virtual size_t get_irqline();
			virtual bool is_exclusive_irq();
		protected:
			size_t irqline = -1;
			bool exclusive_irq = false;
	};
}
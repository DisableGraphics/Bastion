#pragma once
#include <kernel/hal/drvbase/irqc.hpp>

namespace hal {
	class IRQControllerManager final {
		public:
			static IRQControllerManager& get();
			void register_controller(IRQController* controller);
			void register_driver(hal::Driver* device, size_t irqline);
			void enable_irq(size_t irqline);
			void disable_irq(size_t irqline);
			void eoi(size_t irqline);

			[[gnu::no_caller_saved_registers]]
			void irq_handler(size_t irqline);
			void set_irq_for_controller(IRQController*, size_t irqline);
			size_t get_default_irq(hal::Device dev);
			size_t assign_irq(hal::Driver* device);
		private:
			IRQController* find_controller(size_t irqline);
			/// 256 IDT entries - 32 exceptions
			static constexpr size_t MAX_IRQS = 256 - 32;
			IRQController* irq_lines[MAX_IRQS] = {0};
			Vector<IRQController*> controllers;

			IRQControllerManager();
			IRQControllerManager(const IRQControllerManager& other) = delete;
			IRQControllerManager(IRQControllerManager&& other) = delete;
			void operator=(const IRQControllerManager& other) = delete;
			void operator=(IRQControllerManager&& other) = delete;
			
			void isr_setup();
	};
}
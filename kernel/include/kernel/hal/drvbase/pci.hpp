#pragma once
#include <kernel/hal/drvbase/driver.hpp>
#include <stdint.h>
#include <kernel/hal/managers/pci.hpp>

namespace hal {
	class PCIDriver : public Driver {
		public:
			PCIDriver(const PCISubsystemManager::PCIDevice &device);
			virtual void basic_setup();
		protected:
			// PCI device and info
			PCISubsystemManager::PCIDevice device;
			uint32_t bars[6];
	};
}
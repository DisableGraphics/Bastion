#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace hal {
	/**
		\brief PCI Driver.
		\details Implemented as a singleton.
	*/
	class PCISubsystemManager {
		public:
			static PCISubsystemManager& get();

			void init();

			struct PCIDevice {
				uint8_t bus;
				uint8_t device;
				uint8_t function;
				uint16_t vendorID;
				uint16_t deviceID;
				uint8_t class_code;
				uint8_t subclass_code;
				uint8_t prog_if;
			};

			const PCIDevice* search_device(size_t dev_class, size_t dev_subclass);

			// Get the detected device count
			size_t getDeviceCount() const { return deviceCount; }

			// Get device information
			const PCIDevice* getDevices() const { return devices; }
			uint32_t getBAR(uint8_t bus, uint8_t device, uint8_t function, uint8_t barIndex);

			void writeConfigWord(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t data);
			uint16_t readConfigWord(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
		private:
			PCISubsystemManager() { memset(devices, 0, MAX_DEVICES*sizeof(PCIDevice));}

			void checkBus(uint8_t bus);
			void checkDevice(uint8_t bus, uint8_t device);
			void checkFunction(uint8_t bus, uint8_t device, uint8_t function);
			void checkAllBuses();

			uint16_t getVendorID(uint8_t bus, uint8_t device, uint8_t function);
			uint16_t getDeviceID(uint8_t bus, uint8_t device, uint8_t function);
			uint8_t getHeaderType(uint8_t bus, uint8_t device, uint8_t function);
			uint8_t getClassCode(uint8_t bus, uint8_t device, uint8_t function);
			uint8_t getSubclassCode(uint8_t bus, uint8_t device, uint8_t function);
			uint8_t getProgIF(uint8_t bus, uint8_t device, uint8_t function);

			void addDevice(uint8_t bus, uint8_t device, uint8_t function);
			static constexpr size_t MAX_DEVICES = 256;
			PCIDevice devices[MAX_DEVICES];
			size_t deviceCount = 0;
	};
}
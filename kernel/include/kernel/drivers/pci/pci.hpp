#include <stdint.h>
#include <stddef.h>

class PCI {
	public:
		static PCI& get();

		// Initialize PCI subsystem
		void init();

		// Structure to hold device information
		struct PCIDevice {
			uint8_t bus;
			uint8_t device;
			uint8_t function;
			uint16_t vendorID;
			uint16_t deviceID;
			uint8_t class_code;
			uint8_t subclass_code;
		};

		// Get the detected device count
		size_t getDeviceCount() const { return deviceCount; }

		// Get device information
		const PCIDevice* getDevices() const { return devices; }

	private:
		// Private constructor
		PCI() {}

		void checkBus(uint8_t bus);
		void checkDevice(uint8_t bus, uint8_t device);
		void checkFunction(uint8_t bus, uint8_t device, uint8_t function);
		void checkAllBuses();

		uint16_t getVendorID(uint8_t bus, uint8_t device, uint8_t function);
		uint16_t getDeviceID(uint8_t bus, uint8_t device, uint8_t function);
		uint8_t getHeaderType(uint8_t bus, uint8_t device, uint8_t function);
		uint8_t getClassCode(uint8_t bus, uint8_t device, uint8_t function);
		uint8_t getSubclassCode(uint8_t bus, uint8_t device, uint8_t function);

		// Helper functions
		uint16_t readConfigWord(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

		// Fixed-size array to store devices
		static constexpr size_t MAX_DEVICES = 256; // Arbitrary limit, can adjust as needed
		PCIDevice devices[MAX_DEVICES];
		size_t deviceCount = 0;
};
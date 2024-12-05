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
		};

		// Get the detected device count
		size_t getDeviceCount() const { return deviceCount; }

		// Get device information
		const PCIDevice* getDevices() const { return devices; }

	private:
		// Private constructor
		PCI() {}

		// Helper functions
		uint16_t readConfigWord(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

		// Fixed-size array to store devices
		static constexpr size_t MAX_DEVICES = 256; // Arbitrary limit, can adjust as needed
		PCIDevice devices[MAX_DEVICES];
		size_t deviceCount = 0;
};
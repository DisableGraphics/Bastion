#include <kernel/drivers/pci/pci.hpp>
#include <kernel/assembly/inlineasm.h>

PCI& PCI::get() {
	static PCI instance;
	return instance;
}

void PCI::init() {
    deviceCount = 0; // Reset device count
    for (uint16_t bus = 0; bus < 256; ++bus) {
        for (uint8_t device = 0; device < 32; ++device) {
            for (uint8_t function = 0; function < 8; ++function) {
                if (deviceCount >= MAX_DEVICES) return; // Prevent overflow

                // Read Vendor ID to check if a device is present
                uint16_t vendorID = readConfigWord(bus, device, function, 0x00);
                if (vendorID == 0xFFFF) continue; // No device present

                // Read Device ID
                uint16_t deviceID = readConfigWord(bus, device, function, 0x02);

                // Store the device information
                devices[deviceCount++] = {static_cast<uint8_t>(bus), device, function, vendorID, deviceID};

                // If function 0 doesn't support multi-function, skip remaining functions
                if (function == 0) {
                    uint8_t headerType = (readConfigWord(bus, device, function, 0x0C) >> 8) & 0x7F;
                    if (!(headerType & 0x80)) break;
                }
            }
        }
    }
}

uint16_t PCI::readConfigWord(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = (1 << 31) // Enable bit
                     | (bus << 16)
                     | (device << 11)
                     | (function << 8)
                     | (offset & 0xFC);
    outl(0xCF8, address); // Write to CONFIG_ADDRESS port
    return static_cast<uint16_t>(inl(0xCFC) >> ((offset & 2) * 8));
}
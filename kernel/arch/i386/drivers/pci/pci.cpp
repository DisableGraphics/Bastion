#include <kernel/drivers/pci/pci.hpp>
#include <kernel/assembly/inlineasm.h>

#include "../arch/i386/defs/pci/addresses.hpp"

PCI& PCI::get() {
	static PCI instance;
	return instance;
}

void PCI::init() {
	deviceCount = 0;
	for (uint16_t bus = 0; bus < 256; ++bus) {
		checkBus(bus);
	}
}

void PCI::checkBus(uint8_t bus) {
    for (uint8_t device = 0; device < 32; ++device) {
        checkDevice(bus, device);
    }
}

void PCI::checkDevice(uint8_t bus, uint8_t device) {
    uint8_t function = 0;
    uint16_t vendorID = getVendorID(bus, device, function);
    if (vendorID == 0xFFFF) return; // No device present

    checkFunction(bus, device, function);

    uint8_t headerType = getHeaderType(bus, device, function);
    if ((headerType & 0x80) != 0) {
        for (function = 1; function < 8; ++function) {
            if (getVendorID(bus, device, function) != 0xFFFF) {
                checkFunction(bus, device, function);
            }
        }
    }
}

void PCI::checkFunction(uint8_t bus, uint8_t device, uint8_t function) {
    if (deviceCount >= MAX_DEVICES) return;

    uint16_t vendorID = getVendorID(bus, device, function);
    uint16_t deviceID = getDeviceID(bus, device, function);

    devices[deviceCount++] = {bus, device, function, vendorID, deviceID};
}

uint16_t PCI::getVendorID(uint8_t bus, uint8_t device, uint8_t function) {
    return readConfigWord(bus, device, function, 0x00);
}

uint16_t PCI::getDeviceID(uint8_t bus, uint8_t device, uint8_t function) {
    return readConfigWord(bus, device, function, 0x02);
}

uint8_t PCI::getHeaderType(uint8_t bus, uint8_t device, uint8_t function) {
    return static_cast<uint8_t>(readConfigWord(bus, device, function, 0x0C) >> 8);
}

uint16_t PCI::readConfigWord(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = (1 << 31) // Enable bit
                     | (bus << 16)
                     | (device << 11)
                     | (function << 8)
                     | (offset & 0xFC);
    outl(CONFIG_ADDRESS, address); // Write to CONFIG_ADDRESS port
    return static_cast<uint16_t>(inl(CONFIG_DATA) >> ((offset & 2) * 8));
}
#include <kernel/drivers/pci/pci.hpp>
#include <kernel/assembly/inlineasm.h>

#include "../arch/i386/defs/pci/addresses.hpp"

PCI& PCI::get() {
	static PCI instance;
	return instance;
}

void PCI::init() {
	deviceCount = 0;
	checkAllBuses();
}

void PCI::checkAllBuses(void) {
	uint8_t function;
	uint8_t bus;

	uint8_t headerType = getHeaderType(0, 0, 0);
	if ((headerType & 0x80) == 0) {
		// Single PCI host controller
		checkBus(0);
	} else {
		// Multiple PCI host controllers
		for (function = 0; function < 8; function++) {
			if (getVendorID(0, 0, function) != 0xFFFF) break;
			bus = function;
			checkBus(bus);
		}
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

	addDevice(bus, device, function);
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

uint8_t PCI::getClassCode(uint8_t bus, uint8_t device, uint8_t function) {
	return static_cast<uint8_t>(readConfigWord(bus, device, function, 0x0A) >> 8);
}
uint8_t PCI::getSubclassCode(uint8_t bus, uint8_t device, uint8_t function) {
	return static_cast<uint8_t>(readConfigWord(bus, device, function, 0x0A));
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

const PCI::PCIDevice* PCI::search_device(size_t dev_class, size_t dev_subclass) {
	for(size_t i = 0; i < deviceCount; i++) {
		if(devices[i].class_code == dev_class && devices[i].subclass_code == dev_subclass) 
			return &devices[i];
	}
	return nullptr;
}

uint32_t PCI::getBAR(uint8_t bus, uint8_t device, uint8_t function, uint8_t barIndex) {
    uint8_t offset = 0x10 + (barIndex * 4); // BARs start at 0x10
    return readConfigWord(bus, device, function, offset);
}

uint8_t PCI::getProgIF(uint8_t bus, uint8_t device, uint8_t function) {
	return static_cast<uint8_t>(readConfigWord(bus, device, function, 0x08) >> 8);
}

void PCI::addDevice(uint8_t bus, uint8_t device, uint8_t function) {
	uint16_t vendorID = getVendorID(bus, device, function);
	uint16_t deviceID = getDeviceID(bus, device, function);
	uint8_t class_code = getClassCode(bus, device, function);
	uint8_t subclass_code = getSubclassCode(bus, device, function);
	uint8_t prog_if = getProgIF(bus, device, function);

	devices[deviceCount++] = {bus, device, function, 
	vendorID, deviceID,
	class_code, subclass_code, 
	prog_if};
}
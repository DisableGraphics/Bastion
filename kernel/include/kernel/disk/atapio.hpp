#pragma once
#include <kernel/assembly/inlineasm.h>
#include <stdio.h>
#ifdef __i386
#include "../arch/i386/defs/ata/cmd.hpp"
#include "../arch/i386/defs/ata/ports.hpp"
#include "../arch/i386/defs/ata/status_flags.hpp"
#endif

class ATA {
	public:
		static ATA &get();
		void init();
		bool identify();
		bool read_sector(uint32_t lba, uint16_t* buffer);
		bool write_sector(uint32_t lba, const uint16_t* buffer);
	private:
		bool wait_ready();
		uint16_t identity_buffer[256];
		ATA(){};
};

bool ata_wait_ready() {
    while (inb(ATA_STATUS_REG) & ATA_STATUS_BSY);
    return true;
}

// Identify the drive
bool ata_identify(uint16_t* buffer) {
    
}

// Read a sector from the drive
bool ata_read_sector(uint32_t lba, uint16_t* buffer) {
    ata_wait_ready();
    outb(ATA_DRIVE_HEAD_REG, 0xE0 | ((lba >> 24) & 0x0F)); // LBA mode, master drive
    outb(ATA_SECTOR_COUNT_REG, 1); // Read 1 sector
    outb(ATA_LBA_LOW_REG, (uint8_t)(lba & 0xFF)); // LBA low byte
    outb(ATA_LBA_MID_REG, (uint8_t)((lba >> 8) & 0xFF)); // LBA mid byte
    outb(ATA_LBA_HIGH_REG, (uint8_t)((lba >> 16) & 0xFF)); // LBA high byte
    outb(ATA_COMMAND_REG, ATA_CMD_READ_SECTORS); // Send READ command

    while (!(inb(ATA_STATUS_REG) & ATA_STATUS_DRQ)); // Wait for data

    // Read 256 words (512 bytes) of data
    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(ATA_DATA_REG);
    }
    return true;
}

bool ata_write_sector(uint32_t lba, const uint16_t* buffer) {
    // Wait for the drive to be ready
    ata_wait_ready();

    // Set the drive to LBA mode and select the master drive
    outb(ATA_DRIVE_HEAD_REG, 0xE0 | ((lba >> 24) & 0x0F)); // LBA mode, master drive

    // Specify the number of sectors to write (1 in this case)
    outb(ATA_SECTOR_COUNT_REG, 1);

    // Set the LBA address
    outb(ATA_LBA_LOW_REG, (uint8_t)(lba & 0xFF));        // LBA low byte
    outb(ATA_LBA_MID_REG, (uint8_t)((lba >> 8) & 0xFF)); // LBA mid byte
    outb(ATA_LBA_HIGH_REG, (uint8_t)((lba >> 16) & 0xFF)); // LBA high byte

    // Send the WRITE SECTORS command
    outb(ATA_COMMAND_REG, 0x30); // ATA_CMD_WRITE_SECTORS

    // Wait for the drive to request data (DRQ set)
    while (!(inb(ATA_STATUS_REG) & ATA_STATUS_DRQ)) {
        if (inb(ATA_STATUS_REG) & ATA_STATUS_ERR) {
            return false; // Abort on error
        }
    }

    // Write 256 words (512 bytes) of data to the Data Register
    for (int i = 0; i < 256; i++) {
        outw(ATA_DATA_REG, buffer[i]);
    }

    // Wait for the write operation to complete
    ata_wait_ready();

    // Check for errors
    if (inb(ATA_STATUS_REG) & ATA_STATUS_ERR) {
        return false; // Write failed
    }

    return true; // Write successful
}

void ata_test_write() {
    uint16_t write_buffer[256];
    
    // Fill the buffer with example data
    for (int i = 0; i < 256; i++) {
        write_buffer[i] = 0xABCD; // Example pattern
    }

    // Write to sector 1
    if (ata_write_sector(1, write_buffer)) {
        printf("Sector 1 write successful!\n");
    } else {
        printf("Sector 1 write failed!\n");
    }
}

// Test the driver
void ata_test() {
    uint16_t identify_buffer[256];
    if (ata_identify(identify_buffer)) {
        // Drive identified, print some info
        char model[41];
        for (int i = 0; i < 40; i += 2) {
            model[i] = identify_buffer[27 + i / 2] >> 8; // High byte
            model[i + 1] = identify_buffer[27 + i / 2] & 0xFF; // Low byte
        }
        model[40] = '\0';
        printf("Drive Model: %s\n", model);
    } else {
        printf("No drive detected!\n");
    }

    uint16_t read_buffer[256];
    if (ata_read_sector(0, read_buffer)) {
        printf("Sector 0 Data:\n");
        for (int i = 0; i < 256; i++) {
            printf("%p ", read_buffer[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
    } else {
        printf("Failed to read sector!\n");
    }
}
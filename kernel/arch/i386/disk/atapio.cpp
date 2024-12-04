#include <kernel/disk/atapio.hpp>

ATA &ATA::get() {
	static ATA ata;
	return ata;
}

void ATA::init() {
	identify();
}

bool ATA::identify() {
	ata_wait_ready();
    outb(ATA_DRIVE_HEAD_REG, 0xA0); // Select master drive
    outb(ATA_SECTOR_COUNT_REG, 0); // Sector count = 0
    outb(ATA_LBA_LOW_REG, 0); // LBA Low = 0
    outb(ATA_LBA_MID_REG, 0); // LBA Mid = 0
    outb(ATA_LBA_HIGH_REG, 0); // LBA High = 0
    outb(ATA_COMMAND_REG, ATA_CMD_IDENTIFY); // Send IDENTIFY command

    if (inb(ATA_STATUS_REG) == 0) {
        return false; // No drive connected
    }

    while (!(inb(ATA_STATUS_REG) & ATA_STATUS_DRQ)); // Wait for data

    // Read 256 words (512 bytes) of data
    for (int i = 0; i < 256; i++) {
        identity_buffer[i] = inw(ATA_DATA_REG);
    }
    return true;
}
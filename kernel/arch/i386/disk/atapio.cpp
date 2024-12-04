#include <kernel/disk/atapio.hpp>

ATA &ATA::get() {
	static ATA ata;
	return ata;
}

void ATA::init() {
	identify();
}

bool ATA::wait_ready() {
	while (inb(ATA_STATUS_REG) & ATA_STATUS_BSY)
	;
    return true;
}

bool ATA::identify() {
	wait_ready();
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

bool ATA::read_sector(uint32_t lba, uint16_t* buffer) {
	wait_ready();
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

bool ATA::write_sector(uint32_t lba, const uint16_t* buffer) {
	// Wait for the drive to be ready
    wait_ready();

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
    wait_ready();

    // Check for errors
    if (inb(ATA_STATUS_REG) & ATA_STATUS_ERR) {
        return false; // Write failed
    }

    return true; // Write successful
}
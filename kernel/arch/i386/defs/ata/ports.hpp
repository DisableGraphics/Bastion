#pragma once
/// I/O Ports for the Primary ATA Controller
#define ATA_PRIMARY_IO 0x1F0
#define ATA_PRIMARY_CONTROL 0x3F6
#define ATA_DATA_REG (ATA_PRIMARY_IO + 0) // Data register (16-bit)
#define ATA_ERROR_REG (ATA_PRIMARY_IO + 1) // Error register
#define ATA_SECTOR_COUNT_REG (ATA_PRIMARY_IO + 2) // Sector count register
#define ATA_LBA_LOW_REG (ATA_PRIMARY_IO + 3) // LBA low byte
#define ATA_LBA_MID_REG (ATA_PRIMARY_IO + 4) // LBA mid byte
#define ATA_LBA_HIGH_REG (ATA_PRIMARY_IO + 5) // LBA high byte
#define ATA_DRIVE_HEAD_REG (ATA_PRIMARY_IO + 6) // Drive/head register
#define ATA_STATUS_REG (ATA_PRIMARY_IO + 7) // Status register
#define ATA_COMMAND_REG (ATA_PRIMARY_IO + 7) // Command register
#define ATA_ALT_STATUS_REG (ATA_PRIMARY_CONTROL + 0) // Alternate status register
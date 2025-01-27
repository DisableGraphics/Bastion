#pragma once
/**
	\brief Type of AHCI device connected to a port
 */
enum AHCI_DEV {
	NULLDEV,
	SATA,
	SEMB,
	PM,
	SATAPI
};

/// Codes for device connected in a port
#define	SATA_SIG_ATA	0x00000101	// SATA drive
#define	SATA_SIG_ATAPI	0xEB140101	// SATAPI drive
#define	SATA_SIG_SEMB	0xC33C0101	// Enclosure management bridge
#define	SATA_SIG_PM		0x96690101	// Port multiplier
/*
 * Copyright (c) 2024
 * 	N11 Software. All rights reserved.
 *
 * @N11_PUBLIC_SOURCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the N11 Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of software, or to circumvent, violate,
 * or enable the circumvention or violation of, any terms of a software
 * license agreement.
 *
 * Please obtain a copy of the License using Git at:
 * git clone git://n11.dev/health-files.git
 * Navigate to the following location: npsl/N11_LICENSE_2
 * and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND N11 HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @N11_PUBLIC_SOURCE_LICENSE_HEADER_END@
 */

#include <Inferno/IO.h>
#include <Inferno/Log.h>

#include <Drivers/Storage/ATA/ATA.h>

static uint8_t lastSelectedDev = ATA_FLOATING_BUS;

uint8_t readStatus(void) {
	return inb(ATA_PRIMARY_ALT_STATUS_PORT);
}

char* readStatusStr(void) {
	uint8_t status = readStatus();
	if (status & ATA_STATUS_BSY) {
		return (char*)"BUSY";
	} else if (status & ATA_STATUS_DRDY) {
		return (char*)"READY";
	} else if (status & ATA_STATUS_DRQ) {
		return (char*)"DATA REQUEST";
	} else if (status & ATA_STATUS_ERR) {
		return (char*)"ERROR";
	} else if (status & ATA_STATUS_DF) {
		return (char*)"DEV FAULT";
	} else {
		return (char*)"UNK";
	}
}

void waitNotBusy(void) {
	while (readStatus() & ATA_STATUS_BSY);
}

void waitReady(void) {
	while (!(readStatus() & ATA_STATUS_DRDY));
}

void selectDev(uint8_t dev) {
	int i;

	prDebug("ata", "selectDev()");

	if (dev != lastSelectedDev) {
		outb(ATA_PRIMARY_DRIVE_PORT, dev);
		lastSelectedDev = dev;

		for (i = 0; i < 15; i++) {
			inb(ATA_PRIMARY_ALT_STATUS_PORT);
		}
	}
}

int identDev(AtaIDDeviceData* devInfo) {
	int i;

	selectDev(ATA_MASTER_DRIVE);
	waitNotBusy();

	outb(ATA_PRIMARY_SECTOR_COUNT_PORT, 0);
	outb(ATA_PRIMARY_LBA_LOW_PORT, 0);
	outb(ATA_PRIMARY_LBA_MID_PORT, 0);
	outb(ATA_PRIMARY_LBA_HIGH_PORT, 0);

	uint8_t status = readStatus();
	if (status == 0) {
		prWarn("ata", "no device detected");
		return 0;
	}

	waitNotBusy();

	if (inb(ATA_PRIMARY_LBA_MID_PORT) != 0 || inb(ATA_PRIMARY_LBA_HIGH_PORT) != 0) {
		prErr("ata", "not an ata device");
		return 0;
	}

	while (1) {
		status = readStatus();

		if (status & ATA_STATUS_ERR) {
			prErr("ata", "error during device identification");
			return 0;
		}

		if (status & ATA_STATUS_DRQ) break;

		// TODO: add timeout error
	}

	uint16_t* data = (uint16_t*)devInfo;
	for (i = 0; i < 20; i+=2) {
		uint8_t tmp = devInfo->SerialNumber[i];
		devInfo->SerialNumber[i] = devInfo->SerialNumber[i+1];
		devInfo->SerialNumber[i+1] = tmp;
	}

	for (i = 0; i < 8; i+=2) {
		uint8_t tmp = devInfo->FirmwareRevision[i];
		devInfo->FirmwareRevision[i] = devInfo->FirmwareRevision[i+1];
		devInfo->FirmwareRevision[i+1] = tmp;
	}

	return 1;
}

int detectDev(void) {
	prDebug("ata","detectDev()");
	selectDev(ATA_MASTER_DRIVE);
	//waitNotBusy();

	uint8_t status = readStatus();
	uint8_t lbaMid = inb(ATA_PRIMARY_LBA_MID_PORT);
	uint8_t lbaHigh = inb(ATA_PRIMARY_LBA_HIGH_PORT);

	if (status == ATA_FLOATING_BUS && lbaMid == ATA_FLOATING_BUS &&
			lbaHigh == ATA_FLOATING_BUS) {
		return 0;
	}

	prDebug("ata", "detectDev end");

	return 1; // drive detected
}

void setupDriveReadWrite(uint8_t dev, uint32_t lba, uint8_t sectorCnt) {
	selectDev(ATA_MASTER_DRIVE);
	waitNotBusy();
	waitReady();

	uint8_t driveByte = (dev == ATA_SLAVE_DRIVE) ? 0xF0 : 0xE0;

	outb(ATA_PRIMARY_DRIVE_PORT, driveByte | ((lba >> 24) & 0x0F));
	outb(ATA_PRIMARY_SECTOR_COUNT_PORT, sectorCnt);
	outb(ATA_PRIMARY_LBA_LOW_PORT, (uint8_t)(lba));
	outb(ATA_PRIMARY_LBA_MID_PORT, (uint8_t)(lba >> 8));
	outb(ATA_PRIMARY_LBA_HIGH_PORT, (uint8_t)(lba >> 16));
}

// TODO: implement read/write sectors

char* getDevModelNum(AtaIDDeviceData *devInfo) {
	char *model = (char*)devInfo->ModelNumber;
	model[39] = '\0';
	return model;
}

char* getDevSerialNum(AtaIDDeviceData *devInfo) {
	char* serial = (char*)devInfo->SerialNumber;
	serial[19] = '\0';
	return serial;
}

char* getDevFirmwareRev(AtaIDDeviceData *devInfo) {
	char *firmware = (char*)devInfo->FirmwareRevision;
	firmware[7] = '\0';
	return firmware;
}

void printAtaDevInfo(AtaIDDeviceData* devInfo) {
	prInfo("ata", "model: %s", getDevModelNum(devInfo));
	prInfo("ata", "serial: %s", getDevSerialNum(devInfo));
	prInfo("ata", "firmware: %s", getDevFirmwareRev(devInfo));
}

void initDisk(void) {
	prInfo("ata", "initializing disk");

	if (!detectDev()) {
		prInfo("ata", "no drive detected");
	}

	prInfo("ata", "disk status: %s", readStatusStr());

	AtaIDDeviceData devInfo;
	if(identDev(&devInfo)) {
		prInfo("ata", "device information:");
		printAtaDevInfo(&devInfo);
		prInfo("ata", "ata disk initialized");
	} else {
		prErr("ata", "failed to initialize ATA disk");
	}
}

//========= Copyright N11 Software, All rights reserved. ============//
//
// File: acpi.h
// Purpose: Main header file for the ACPI driver
// Maintainer: atl
//
//===================================================================//

#pragma once

#include <Inferno/stdint.h>

namespace ACPI {
    // ACPI initialization
    bool init(void* rsdpAddress);
    
    // ACPI power management functions
    bool shutdown();
    bool reboot();
    
    // Structure definitions for ACPI tables
    struct RSDPDescriptor {
        char Signature[8];
        uint8_t Checksum;
        char OEMID[6];
        uint8_t Revision;
        uint32_t RSDTAddress;
    } __attribute__((packed));
    
    struct RSDT {
        char Signature[4];
        uint32_t Length;
        uint8_t Revision;
        uint8_t Checksum;
        char OEMID[6];
        char OEMTableID[8];
        uint32_t OEMRevision;
        uint32_t CreatorID;
        uint32_t CreatorRevision;
        // Variable length array of 32-bit physical addresses
    } __attribute__((packed));
    
    struct FACP {
        char Signature[4];
        uint32_t Length;
        uint8_t Revision;
        uint8_t Checksum;
        char OEMID[6];
        char OEMTableID[8];
        uint32_t OEMRevision;
        uint32_t CreatorID;
        uint32_t CreatorRevision;
        uint32_t FIRMWARE_CTRL;
        uint32_t DSDT;
        uint8_t Reserved;
        uint8_t Preferred_PM_Profile;
        uint16_t SCI_INT;
        uint32_t SMI_CMD;
        uint8_t ACPI_ENABLE;
        uint8_t ACPI_DISABLE;
        uint8_t S4BIOS_REQ;
        uint8_t PSTATE_CNT;
        uint32_t PM1a_EVT_BLK;
        uint32_t PM1b_EVT_BLK;
        uint32_t PM1a_CNT_BLK;
        uint32_t PM1b_CNT_BLK;
        uint32_t PM2_CNT_BLK;
        uint32_t PM_TMR_BLK;
        uint32_t GPE0_BLK;
        uint32_t GPE1_BLK;
        uint8_t PM1_EVT_LEN;
        uint8_t PM1_CNT_LEN;
        uint8_t PM2_CNT_LEN;
        uint8_t PM_TMR_LEN;
        uint8_t GPE0_BLK_LEN;
        uint8_t GPE1_BLK_LEN;
        uint8_t GPE1_BASE;
        uint8_t CST_CNT;
        uint16_t P_LVL2_LAT;
        uint16_t P_LVL3_LAT;
        uint16_t FLUSH_SIZE;
        uint16_t FLUSH_STRIDE;
        uint8_t DUTY_OFFSET;
        uint8_t DUTY_WIDTH;
        uint8_t DAY_ALRM;
        uint8_t MON_ALRM;
        uint8_t CENTURY;
        uint16_t IAPC_BOOT_ARCH;
        uint8_t Reserved2;
        uint32_t Flags;
        uint8_t RESET_REG[12];
        uint8_t RESET_VALUE;
        uint8_t Reserved3[3];
        uint64_t X_FIRMWARE_CTRL;
        uint64_t X_DSDT;
        uint8_t X_PM1a_EVT_BLK[12];
        uint8_t X_PM1b_EVT_BLK[12];
        uint8_t X_PM1a_CNT_BLK[12];
        uint8_t X_PM1b_CNT_BLK[12];
        uint8_t X_PM2_CNT_BLK[12];
        uint8_t X_PM_TMR_BLK[12];
        uint8_t X_GPE0_BLK[12];
        uint8_t X_GPE1_BLK[12];
        uint16_t SLP_TYPa;
        uint16_t SLP_TYPb;
    } __attribute__((packed));
	
	void tryApmShutdown();
	void tryQemuShutdown();
	void tryAcpiShutdown();
}

//========= Copyright N11 Software, All rights reserved. ============//
//
// File: acpi.cpp
// Purpose: Implementation of ACPI functions
// Maintainer: FiReLScar
//
//===================================================================//

#include <Drivers/ACPI/acpi.h>
#include <Inferno/IO.h>
#include <Inferno/Log.h>
#include <Inferno/stdint.h>

namespace ACPI {
    // Global variables to store ACPI information
    static RSDPDescriptor* rsdp = nullptr;
    static RSDT* rsdt = nullptr;
    static FACP* facp = nullptr;
    static bool acpiEnabled = false;

    // Find RSDP in memory
    RSDPDescriptor* findRSDP() {
        // Search for RSDP signature in EBDA and system BIOS areas
        const char* signature = "RSD PTR ";
        
        // Search in EBDA (usually at 0x9FC00 - 0x9FFFF)
        for (uint64_t addr = 0x9FC00; addr < 0xA0000; addr += 16) {
            bool found = true;
            for (int i = 0; i < 8; i++) {
                if (*(char*)(addr + i) != signature[i]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                prInfo("acpi", "Found RSDP at 0x%x", addr);
                return (RSDPDescriptor*)addr;
            }
        }
        
        // Search in BIOS area (0xE0000 - 0xFFFFF)
        for (uint64_t addr = 0xE0000; addr < 0x100000; addr += 16) {
            bool found = true;
            for (int i = 0; i < 8; i++) {
                if (*(char*)(addr + i) != signature[i]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                prInfo("acpi", "Found RSDP at 0x%x", addr);
                return (RSDPDescriptor*)addr;
            }
        }
        
        return nullptr;
    }
    
    // Validate RSDP checksum
    bool validateRSDP(RSDPDescriptor* rsdp) {
        uint8_t sum = 0;
        uint8_t* ptr = (uint8_t*)rsdp;
        
        // Sum all bytes in the structure
        for (int i = 0; i < sizeof(RSDPDescriptor); i++) {
            sum += ptr[i];
        }
        
        // If valid, checksum should be 0
        return sum == 0;
    }
    
    // Find FACP (Fixed ACPI Description Table) from RSDT
    FACP* findFACP() {
        if (!rsdt) return nullptr;
        
        // Calculate the number of entries in the RSDT
        uint32_t entries = (rsdt->Length - sizeof(RSDT)) / 4;
        
        // Get the array of pointers that follows the header
        uint32_t* tables = (uint32_t*)((uint64_t)rsdt + sizeof(RSDT));
        
        // Look for FACP table
        for (uint32_t i = 0; i < entries; i++) {
            void* tableAddr = (void*)(uint64_t)tables[i];
            if (!tableAddr) continue;
            
            // Check if this is a FACP table
            if (*(uint32_t*)tableAddr == *(uint32_t*)"FACP") {
                prInfo("acpi", "Found FACP at 0x%x", (uint64_t)tableAddr);
                return (FACP*)tableAddr;
            }
        }
        
        return nullptr;
    }
    
    // Enable ACPI mode
    bool enableACPI() {
        if (!facp) return false;
        
        // Check if ACPI is already enabled
        if (inw(facp->PM1a_CNT_BLK) & 1) {
            prInfo("acpi", "ACPI is already enabled");
            return true;
        }
        
        // Check if SMI_CMD and ACPI_ENABLE are both valid
        if (facp->SMI_CMD == 0 || facp->ACPI_ENABLE == 0) {
            prErr("acpi", "Cannot enable ACPI - missing SMI_CMD or ACPI_ENABLE");
            return false;
        }
        
        // Enable ACPI by writing ACPI_ENABLE to SMI_CMD
        prInfo("acpi", "Writing ACPI_ENABLE to SMI_CMD");
        outb(facp->SMI_CMD, facp->ACPI_ENABLE);
        
        // Wait for ACPI to become enabled
        for (int i = 0; i < 100; i++) {
            if (inw(facp->PM1a_CNT_BLK) & 1) {
                prInfo("acpi", "ACPI enabled successfully");
                return true;
            }
            // Small delay
            for (volatile int j = 0; j < 1000000; j++);
        }
        
        prErr("acpi", "Failed to enable ACPI");
        return false;
    }
    
    bool init(void* rsdpAddress) {
        prInfo("acpi", "Initializing ACPI...");
        
        // Use the RSDP provided by the bootloader
        rsdp = (RSDPDescriptor*)rsdpAddress;
        if (!rsdp) {
            prErr("acpi", "RSDP not found");
            return false;
        }
        
        // Validate RSDP checksum
        if (!validateRSDP(rsdp)) {
            prErr("acpi", "RSDP checksum invalid");
            return false;
        }
        
        // Get RSDT
        rsdt = (RSDT*)(uint64_t)rsdp->RSDTAddress;
        if (!rsdt) {
            prErr("acpi", "RSDT not found");
            return false;
        }
        
        // Find FACP
        facp = findFACP();
        if (!facp) {
            prErr("acpi", "FACP not found");
            return false;
        }
        
        // Enable ACPI
        acpiEnabled = enableACPI();
        
        prInfo("acpi", "ACPI initialization %s", acpiEnabled ? "successful" : "failed");
        return acpiEnabled;
    }
    
    // Perform an ACPI shutdown
    bool shutdown() {
        if (!acpiEnabled || !facp) {
            prErr("acpi", "ACPI not available for shutdown");
            return false;
        }
        
        prInfo("acpi", "Performing ACPI shutdown...");

        // Try standard ACPI shutdown first
        tryAcpiShutdown();
        
        // If that failed, try QEMU/Bochs specific methods
        tryQemuShutdown();
        
        // If that failed, try APM shutdown
        tryApmShutdown();
        
        // If we get here, shutdown failed
        prErr("acpi", "All shutdown methods failed");
        return false;
    }
    
    // Try standard ACPI shutdown
    void tryAcpiShutdown() {
        prInfo("acpi", "Trying standard ACPI shutdown...");
        
        // Read current PM1a_CNT_BLK value
        uint16_t pm1a_cnt = inw(facp->PM1a_CNT_BLK);
        prInfo("acpi", "Current PM1a_CNT_BLK: 0x%x", pm1a_cnt);
        
        // Use hardcoded S5 sleep type values (0x5) for QEMU compatibility
        uint16_t slp_typa = 0x5;
        uint16_t slp_typb = 0x5;
        
        // Try with SLP values from FACP if they look valid
        if ((facp->SLP_TYPa & 0x1F) != 0) {
            slp_typa = facp->SLP_TYPa & 0x1F;
        }
        if ((facp->SLP_TYPb & 0x1F) != 0) {
            slp_typb = facp->SLP_TYPb & 0x1F;
        }
        
        prInfo("acpi", "Using SLP_TYPa: 0x%x, SLP_TYPb: 0x%x", slp_typa, slp_typb);
        
        // Write SLP_TYPa and SLP_EN (1<<13) to PM1a_CNT_BLK
        uint16_t shutdown_value = (pm1a_cnt & 0xE3FF) | (slp_typa << 10) | (1 << 13);
        prInfo("acpi", "Writing 0x%x to PM1a_CNT_BLK", shutdown_value);
        outw(facp->PM1a_CNT_BLK, shutdown_value);
        
        // If there's a second control block, also write to it
        if (facp->PM1b_CNT_BLK != 0) {
            uint16_t pm1b_cnt = inw(facp->PM1b_CNT_BLK);
            prInfo("acpi", "Current PM1b_CNT_BLK: 0x%x", pm1b_cnt);
            uint16_t shutdown_value_b = (pm1b_cnt & 0xE3FF) | (slp_typb << 10) | (1 << 13);
            prInfo("acpi", "Writing 0x%x to PM1b_CNT_BLK", shutdown_value_b);
            outw(facp->PM1b_CNT_BLK, shutdown_value_b);
        }
        
        // Wait a bit to see if shutdown worked
        for (volatile int i = 0; i < 10000000; i++);
        prInfo("acpi", "Standard ACPI shutdown failed");
    }
    
    // Try QEMU/Bochs specific shutdown methods
    void tryQemuShutdown() {
        prInfo("acpi", "Trying QEMU/Bochs shutdown...");
        
        // Bochs/QEMU shutdown port
        outw(0x604, 0x2000);
        for (volatile int i = 0; i < 1000000; i++);
        
        // QEMU newer versions shutdown port
        outw(0xB004, 0x2000);
        for (volatile int i = 0; i < 1000000; i++);
        
        // Try QEMU system reset port with shutdown command
        outw(0x64, 0xFE);
        for (volatile int i = 0; i < 1000000; i++);
        
        prInfo("acpi", "QEMU/Bochs shutdown failed");
    }
    
    // Try APM shutdown
    void tryApmShutdown() {
        prInfo("acpi", "Trying APM shutdown...");
        
        // APM shutdown
        outb(0xF4, 0x10);
        for (volatile int i = 0; i < 1000000; i++);
        
        prInfo("acpi", "APM shutdown failed");
    }

    bool reboot() {
        prInfo("acpi", "Attempting system reboot...");
        
        // Try ACPI reboot first
        if (facp && facp->RESET_REG[0] != 0) {
            prInfo("acpi", "Trying ACPI reset register...");
            outb(facp->RESET_REG[0], facp->RESET_VALUE);
        }
        
        // Try keyboard controller
        prInfo("acpi", "Trying keyboard controller reset...");
        uint8_t good = 0x02;
        while (good & 0x02)
            good = inb(0x64);
        outb(0x64, 0xFE);
        io_wait();
        
        // Try PCI reset
        prInfo("acpi", "Trying PCI reset...");
        outb(0xCF9, 0x0E);
        io_wait();
        
        // Try triple fault
        prInfo("acpi", "Trying triple fault...");
        asm volatile("int $0x03");
        
        // If we get here, nothing worked
        prErr("acpi", "All reboot methods failed");
        return false;
    }
}

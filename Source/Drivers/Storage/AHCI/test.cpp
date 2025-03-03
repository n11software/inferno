//========= Copyright N11 Software, All rights reserved. ============//
//
// File: test.cpp
// Purpose: Test code for AHCI SATA driver
// Maintainer: 
//
//===================================================================//

#include <Drivers/Storage/AHCI/AHCI.h>
#include <Memory/Heap.hpp>
#include <Inferno/Log.h>
#include <Inferno/stdint.h>

// This function demonstrates how to use the AHCI SATA driver
void test_ahci_sata() {
    prInfo("test", "Starting SATA driver test...");
    
    // First, check if AHCI is already initialized by looking for devices
    int already_initialized = 0;
    for (int i = 0; i < AHCI_MAX_PORTS; i++) {
        ahci_device_t* dev = ahci_get_device_info(i);
        if (dev && dev->is_present) {
            already_initialized++;
        }
    }
    
    if (already_initialized > 0) {
        prInfo("test", "AHCI appears to be already initialized with %d devices", already_initialized);
    } else {
        // Initialize the AHCI controller
        // In a real scenario, vendor_id and device_id would come from PCI enumeration
        prInfo("test", "AHCI not initialized, attempting to initialize now...");
        int devices = ahci_init(0, 0); // Let the PCI subsystem find the controller
        
        if (devices <= 0) {
            prErr("test", "No SATA devices found during initialization");
            return;
        }
        
        prInfo("test", "Successfully initialized AHCI with %d SATA devices", devices);
    }
    
    // Find the first available device
    int port_num = -1;
    for (int i = 0; i < AHCI_MAX_PORTS; i++) {
        ahci_device_t* dev = ahci_get_device_info(i);
        if (dev && dev->is_present) {
            port_num = i;
            prInfo("test", "Found device on port %d", i);
            break;
        }
    }
    
    if (port_num < 0) {
        prErr("test", "No available SATA devices found in device info table");
        return;
    }
    
    // Get device info
    ahci_device_t* dev = ahci_get_device_info(port_num);
    if (!dev) {
        prErr("test", "Failed to get device info for port %d", port_num);
        return;
    }
    
    prInfo("test", "=== SATA Device Information ===");
    prInfo("test", "Port: %d", port_num);
    prInfo("test", "Model: %s", dev->model);
    prInfo("test", "Serial: %s", dev->serial);
    prInfo("test", "Firmware: %s", dev->firmware_rev);
    prInfo("test", "Capacity: %llu sectors (%llu MB)",
           dev->sector_count, (dev->sector_count * dev->sector_size) / (1024 * 1024));
    prInfo("test", "Sector Size: %d bytes", dev->sector_size);
    prInfo("test", "================================");
    
    // Detect filesystem type
    prInfo("test", "Detecting filesystem type...");
    int fs_type = ahci_detect_filesystem(port_num);
    if (fs_type < 0) {
        prErr("test", "Failed to detect filesystem");
    } else {
        prInfo("test", "Filesystem detection completed.");
    }
    
    // Allocate buffer for read/write tests
    uint32_t sector_size = dev->sector_size;
    uint8_t* buffer = (uint8_t*)Heap::Allocate(sector_size);
    if (!buffer) {
        prErr("test", "Failed to allocate buffer for read test");
        return;
    }
    
    // Read the first sector (usually contains MBR or partition table)
    prInfo("test", "Reading first sector...");
    int result = ahci_read_sectors(port_num, 0, 1, buffer);
    
    if (result > 0) {
        prInfo("test", "Read successful (%d bytes)", result);
        
        // Display first 32 bytes as hex in a more readable format
        prInfo("test", "First 32 bytes:");
        for (int i = 0; i < 32; i++) {
            if (i % 16 == 0) kprintf("\n  ");
            kprintf("%02X ", buffer[i]);
        }
        kprintf("\n");
        
        // Check for MBR signature (0x55, 0xAA at offset 510, 511)
        if (buffer[510] == 0x55 && buffer[511] == 0xAA) {
            prInfo("test", "Valid MBR signature detected (0x55 0xAA)");
        } else {
            prInfo("test", "No MBR signature (expected 0x55 0xAA, found 0x%02X 0x%02X)", 
                   buffer[510], buffer[511]);
        }
        
        // Write test - WARNING: Uncommenting this would modify disk data!
        /*
        // Modify buffer for writing
        for (int i = 0; i < 16; i++) {
            buffer[i] = i;
        }
        
        prInfo("test", "Writing to sector 100...");
        result = ahci_write_sectors(port_num, 100, 1, buffer);
        
        if (result > 0) {
            prInfo("test", "Write successful (%d bytes)", result);
        } else {
            prErr("test", "Write failed");
        }
        */
    } else {
        prErr("test", "Read failed with error code %d", result);
    }
    
    // Free the buffer
    Heap::Free(buffer);
    prInfo("test", "SATA driver test completed");
}

// This function can be called from main.cpp to test the SATA driver
extern "C" void test_sata_driver() {
    test_ahci_sata();
} 
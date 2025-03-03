//========= Copyright N11 Software, All rights reserved. ============//
//
// File: AHCI.cpp
// Purpose: AHCI controller driver for SATA devices
// Maintainer: 
//
//===================================================================//

#include <Drivers/Storage/AHCI/AHCI.h>
#include <Drivers/PCI/PCI.h>
#include <Drivers/TTY/COM.h>
#include <Memory/Memory.hpp>
#include <Memory/Heap.hpp>
#include <Memory/Paging.hpp>
#include <Memory/Mem_.hpp>
#include <Inferno/Log.h>
#include <Inferno/IO.h>
#include <Inferno/stdint.h>

// ATA commands
#define ATA_CMD_READ_DMA_EXT  0x25
#define ATA_CMD_WRITE_DMA_EXT 0x35
#define ATA_CMD_IDENTIFY      0xEC

// Maximum number of ports supported by the driver
#define AHCI_MAX_PORTS 32

// Define port type alias for easier access has been moved to the header

// AHCI Constants that should be defined globally
#define AHCI_HBA_MEMORY 0xFEBD0000  // This is a common fixed address for AHCI in emulators
#define AHCI_GHC_AHCI_ENABLE 0x80000000  // AHCI Enable bit in GHC register

// At the beginning of the file, add these constants if they don't exist
#define AHCI_PORT_SCTL_DET_INIT    0x1     // Initialize device detection
#define AHCI_PORT_SCTL_DET_DISABLE 0x4     // Disable device detection
#define AHCI_PORT_SCTL_IPM_NONE    0x0     // No power management restrictions

// AHCI controller memory-mapped registers
static volatile ahci_hba_memory_t* hba_memory = nullptr;

// Array of AHCI devices (one per port)
static ahci_device_t ahci_devices[AHCI_MAX_PORTS];

// Memory for command lists and received FIS structures (one per port)
static ahci_cmd_header_t* cmd_list[AHCI_MAX_PORTS];
static ahci_received_fis_t* received_fis[AHCI_MAX_PORTS];
static ahci_cmd_table_t* cmd_tables[AHCI_MAX_PORTS][32]; // 32 command slots per port

// Filesystem type definitions moved to the header file

// Convert a 16-bit word array to a string and trim leading/trailing spaces
static void ata_string_from_words(char* dest, uint16_t* src, int words_count) {
    // ATA strings are packed as little-endian words, so swap bytes
    for (int i = 0; i < words_count; i++) {
        dest[i*2] = (src[i] >> 8) & 0xFF;
        dest[i*2+1] = src[i] & 0xFF;
    }
    
    // Null-terminate
    dest[words_count*2] = '\0';
    
    // Trim trailing spaces
    for (int i = words_count*2 - 1; i >= 0; i--) {
        if (dest[i] == ' ') {
            dest[i] = '\0';
        } else if (dest[i] != '\0') {
            break;
        }
    }
    
    // Trim leading spaces
    int start = 0;
    while (dest[start] == ' ') start++;
    
    if (start > 0) {
        for (int i = 0; dest[start + i] != '\0'; i++) {
            dest[i] = dest[start + i];
        }
    }
}

// Check the type of device connected to a port
int ahci_check_port_type(volatile ahci_hba_memory_t* hba, int port_num) {
    uint32_t ssts = hba->ports[port_num].ssts;
    
    // These should be small values (DET: 0-3, IPM: 0-8)
    // Explicitly mask to get the correct bit fields
    uint8_t det = ssts & 0xF;          // bits 0-3
    uint8_t ipm = (ssts >> 8) & 0xF;   // bits 8-11
    
    uint32_t sig = hba->ports[port_num].sig;
    
    // Enhanced logging
    // prInfo("ahci", "Port %d: SSTS=0x%x (raw)", port_num, ssts);
    // prInfo("ahci", "Port %d: DET=%d, IPM=%d, Signature=0x%x", port_num, det, ipm, sig);
    
    // Check if device is present and communication is established
    if ((det != AHCI_PORT_SSTS_DET_PRESENT && det != AHCI_PORT_SSTS_DET_ESTABLISHED) || 
        ipm != AHCI_PORT_SSTS_IPM_ACTIVE) {
        
        if (det == 0) {
            // prInfo("ahci", "Port %d: No device detected (DET=0)", port_num);
        } else if (det != AHCI_PORT_SSTS_DET_PRESENT && det != AHCI_PORT_SSTS_DET_ESTABLISHED) {
            // prInfo("ahci", "Port %d: Device detection failed (DET=%d, expected 1 or 3)", port_num, det);
        }
        
        if (ipm != AHCI_PORT_SSTS_IPM_ACTIVE) {
            // prInfo("ahci", "Port %d: Device not in active state (IPM=%d, expected 1)", port_num, ipm);
        }
        
        return AHCI_DEV_NULL;
    }
    
    // Check the signature to determine the device type
    switch (sig) {
        case 0x00000101: // SATA drive
            prInfo("ahci", "Port %d: SATA drive signature detected (0x00000101)", port_num);
            return AHCI_DEV_SATA;
        case 0xEB140101: // SATAPI drive
            prInfo("ahci", "Port %d: SATAPI drive signature detected (0xEB140101)", port_num);
            return AHCI_DEV_SATAPI;
        case 0xC33C0101: // Enclosure management bridge
            prInfo("ahci", "Port %d: SEMB device signature detected (0xC33C0101)", port_num);
            return AHCI_DEV_SEMB;
        case 0x96690101: // Port multiplier
            prInfo("ahci", "Port %d: Port Multiplier signature detected (0x96690101)", port_num);
            return AHCI_DEV_PM;
        default:
            // Special case for QEMU - sometimes valid devices have non-standard signatures
            if (det == AHCI_PORT_SSTS_DET_PRESENT || det == AHCI_PORT_SSTS_DET_ESTABLISHED) {
                // prInfo("ahci", "Port %d: Non-standard signature (0x%x) but device present, assuming SATA", 
                    //    port_num, sig);
                return AHCI_DEV_SATA;
            }
            
            prInfo("ahci", "Port %d: Unknown device signature: 0x%x", port_num, sig);
            return AHCI_DEV_NULL;
    }
}

// Find a free command slot in the command list
int ahci_find_command_slot(volatile ahci_hba_memory_t* hba, int port_num) {
    // Get the slots that are free (not in use)
    uint32_t slots = (hba->ports[port_num].sact | hba->ports[port_num].ci);
    
    // There are 32 slots in total
    for (int i = 0; i < 32; i++) {
        if (!(slots & (1 << i))) {
            return i;
        }
    }
    
    // No free slots
    prErr("ahci", "Cannot find free command slot");
    return -1;
}

// Stop command processing on a port
int ahci_port_stop_cmd(volatile ahci_hba_memory_t* hba, int port_num) {
    // Clear ST bit
    hba->ports[port_num].cmd &= ~AHCI_PORT_CMD_ST;
    hba->ports[port_num].cmd &= ~AHCI_PORT_CMD_FRE;
    
    // Wait until FR and CR are cleared
    uint64_t timeout = 5000000; // 5 seconds (increased from 500ms)
    while (timeout--) {
        if ((hba->ports[port_num].cmd & AHCI_PORT_CMD_FR) ||
            (hba->ports[port_num].cmd & AHCI_PORT_CMD_CR)) {
            // Small delay - increased for emulated environment
            for (int i = 0; i < 10000; i++) { __asm__ volatile("nop"); }
        } else {
            break;
        }
    }
    
    if (timeout == 0) {
        prErr("ahci", "Port %d failed to stop commands", port_num);
        return -1;
    }
    
    return 0;
}

// Start command processing on a port
int ahci_port_start_cmd(volatile ahci_hba_memory_t* hba, int port_num) {
    // Wait until CR is cleared
    while (hba->ports[port_num].cmd & AHCI_PORT_CMD_CR);
    
    // Set FRE and ST
    hba->ports[port_num].cmd |= AHCI_PORT_CMD_FRE;
    hba->ports[port_num].cmd |= AHCI_PORT_CMD_ST;
    
    return 0;
}

// Initialize and start a port
void ahci_port_rebase(int port_num) {
    // Stop command processing
    ahci_port_stop_cmd(hba_memory, port_num);
    
    // Allocate memory for command list (1KB aligned)
    // In a real OS, we would use a memory allocator that returns aligned memory
    cmd_list[port_num] = (ahci_cmd_header_t*)Heap::Allocate(1024);
    memset(cmd_list[port_num], 0, 1024);
    
    // Allocate memory for received FIS (256 bytes aligned)
    received_fis[port_num] = (ahci_received_fis_t*)Heap::Allocate(256);
    memset((void*)received_fis[port_num], 0, 256);
    
    // Allocate memory for command tables (128 bytes aligned, includes PRDT)
    for (int i = 0; i < 32; i++) {
        cmd_tables[port_num][i] = (ahci_cmd_table_t*)Heap::Allocate(256);
        memset(cmd_tables[port_num][i], 0, 256);
    }
    
    // Set command list and FIS base addresses
    // Note: For simplicity, we're assuming physical == virtual address
    // In a real OS with paging, you'd need to convert virtual to physical
    hba_memory->ports[port_num].clb = (uint32_t)(uint64_t)cmd_list[port_num];
    hba_memory->ports[port_num].clbu = 0; // Upper 32 bits, assume 32-bit addressing
    
    hba_memory->ports[port_num].fb = (uint32_t)(uint64_t)received_fis[port_num];
    hba_memory->ports[port_num].fbu = 0; // Upper 32 bits
    
    // Initialize command list headers
    for (int i = 0; i < 32; i++) {
        cmd_list[port_num][i].prdbc = 0;
        cmd_list[port_num][i].ctba = (uint32_t)(uint64_t)cmd_tables[port_num][i];
        cmd_list[port_num][i].ctbau = 0; // Upper 32 bits
    }
    
    // Start command processing
    ahci_port_start_cmd(hba_memory, port_num);
    
    // Clear any pending interrupts
    hba_memory->ports[port_num].is = 0xFFFFFFFF;
    
    // Enable interrupts
    hba_memory->ports[port_num].ie = 0xFD000000; // Enable all error interrupts
    hba_memory->ports[port_num].ie |= (1 << 5);  // And also DPS
}

// Identify the device connected to a port
int ahci_identify_device(int port_num) {
    if (port_num >= AHCI_MAX_PORTS || !hba_memory) {
        prErr("ahci", "Identify device failed: Invalid port number or HBA not initialized");
        return -1;
    }
    
    // prInfo("ahci", "Identifying device on port %d...", port_num);
    
    volatile ahci_hba_port_t* port = &hba_memory->ports[port_num];
    
    // Ensure port has a device connected
    uint32_t ssts = port->ssts;
    uint8_t det = ssts & AHCI_PORT_SSTS_DET_MASK;
    uint8_t ipm = (ssts >> 8) & 0x0F;
    
    // prInfo("ahci", "Port %d SSTS: 0x%08x (DET=%d, IPM=%d)", port_num, ssts, det, ipm);
    
    if ((port->ssts & AHCI_PORT_SSTS_DET_MASK) != (AHCI_PORT_SSTS_DET_ESTABLISHED << AHCI_PORT_SSTS_DET_SHIFT)) {
        prErr("ahci", "Identify device failed: No device detected or communication not established");
        return -1;
    }
    
    // Allocate buffer for identify data (sector size is 512 bytes)
    uint16_t* identify_data = (uint16_t*)Heap::Allocate(512);
    if (!identify_data) {
        prErr("ahci", "Failed to allocate buffer for identify data");
        return -1;
    }
    
    // prInfo("ahci", "Finding free command slot...");
    
    // Find a free command slot
    int slot = ahci_find_command_slot(hba_memory, port_num);
    if (slot < 0) {
        prErr("ahci", "Failed to find free command slot");
        Heap::Free(identify_data);
        return -1;
    }
    
    // prInfo("ahci", "Using command slot %d", slot);
    
    // Setup command header
    ahci_cmd_header_t* cmd_hdr = &cmd_list[port_num][slot];
    cmd_hdr->dw0 = sizeof(fis_reg_h2d_t) / sizeof(uint32_t); // Command FIS size in DWORDs (5)
    cmd_hdr->dw0 |= (1 << 7); // Prefetchable
    cmd_hdr->prdbc = 0;
    
    // Setup command table
    ahci_cmd_table_t* cmd_tbl = cmd_tables[port_num][slot];
    memset(cmd_tbl, 0, sizeof(ahci_cmd_table_t));
    
    // Setup physical region descriptor (PRD)
    cmd_tbl->prdt[0].dba = (uint32_t)(uint64_t)identify_data;
    cmd_tbl->prdt[0].dbau = 0; // Upper 32 bits
    cmd_tbl->prdt[0].dbc = 511; // 512 bytes (0 based)
    cmd_tbl->prdt[0].dbc |= AHCI_PRD_DBC_INTERRUPT; // Interrupt on completion
    
    // prInfo("ahci", "Setting up IDENTIFY command FIS...");
    
    // Setup command FIS
    fis_reg_h2d_t* cmd_fis = (fis_reg_h2d_t*)cmd_tbl->cfis;
    memset(cmd_fis, 0, sizeof(fis_reg_h2d_t));
    
    cmd_fis->fis_type = FIS_TYPE_REG_H2D;
    cmd_fis->c = 1; // Command
    cmd_fis->command = ATA_CMD_IDENTIFY;
    cmd_fis->device = 0; // LBA mode
    
    // prInfo("ahci", "Issuing IDENTIFY command...");
    
    // Issue the command
    port->ci = 1 << slot;
    
    // prInfo("ahci", "Waiting for command completion...");
    
    // Wait for completion
    uint64_t timeout = 5000000; // Increase timeout to 5 seconds (was 500ms)
    while (timeout--) {
        if (!(port->ci & (1 << slot))) {
            // Command completed
            // prInfo("ahci", "IDENTIFY command completed successfully");
            break;
        }
        
        // Check for errors
        if (port->is & AHCI_PORT_INT_TFES) {
            prErr("ahci", "IDENTIFY command error on port %d (IS=0x%08x, TFD=0x%08x)", 
                 port_num, port->is, port->tfd);
            Heap::Free(identify_data);
            return -1;
        }
        
        // Small delay - increase delay for emulated environment
        for (int i = 0; i < 10000; i++) { __asm__ volatile("nop"); }
    }
    
    if (timeout == 0) {
        // prErr("ahci", "IDENTIFY command timeout on port %d (IS=0x%08x, TFD=0x%08x)", 
            //  port_num, port->is, port->tfd);
        Heap::Free(identify_data);
        return -1;
    }
    
    // prInfo("ahci", "Processing IDENTIFY data...");
    
    // Process identify data
    ahci_device_t* dev = &ahci_devices[port_num];
    dev->port_num = port_num;
    dev->device_type = AHCI_DEV_SATA;
    dev->is_present = 1;
    
    // Get model, serial, firmware information
    ata_string_from_words(dev->model, &identify_data[27], 20);
    ata_string_from_words(dev->serial, &identify_data[10], 10);
    ata_string_from_words(dev->firmware_rev, &identify_data[23], 4);
    
    // Get sector count (words 100-103 for 48-bit LBA)
    if (identify_data[83] & (1 << 10)) { // LBA48 supported
        dev->sector_count = *(uint64_t*)&identify_data[100];
        // prInfo("ahci", "Device supports 48-bit LBA addressing");
    } else {
        dev->sector_count = *(uint32_t*)&identify_data[60];
        // prInfo("ahci", "Device supports 28-bit LBA addressing");
    }
    
    // Set sector size (512 bytes by default)
    dev->sector_size = 512;
    
    // Check for logical sector size information (words 106, 117-118)
    if ((identify_data[106] & 0xE000) == 0x6000) {
        // Logical sector size > 512 bytes
        if (identify_data[106] & (1 << 12)) {
            dev->sector_size = *(uint32_t*)&identify_data[117];
        }
    }
    
    // Ensure we have valid values - if not, use defaults for QEMU
    if (dev->sector_count == 0 || dev->sector_count > 0xFFFFFFFFFFFFFFFF) {
        prWarn("ahci", "Invalid sector count detected, using default value for QEMU");
        dev->sector_count = 204800; // 100 MB default size for QEMU
    }
    
    if (dev->sector_size == 0 || dev->sector_size > 8192) {
        prInfo("ahci", "Invalid sector size detected, using default value");
        dev->sector_size = 512; // Standard sector size
    }
    
    // Debug raw values
    // prInfo("ahci", "Raw sector count words: 0x%04x 0x%04x 0x%04x 0x%04x", 
        //    identify_data[100], identify_data[101], identify_data[102], identify_data[103]);
    // prInfo("ahci", "Raw sector size words: 0x%04x 0x%04x 0x%04x", 
        //    identify_data[106], identify_data[117], identify_data[118]);
    
    // prInfo("ahci", "Port %d: %s (%s), %lu sectors, %u bytes per sector",
    //        port_num, dev->model, dev->serial, 
    //        (unsigned long)dev->sector_count, dev->sector_size);
    
    Heap::Free(identify_data);
    return 0;
}

// Initialize a port
int ahci_port_init(int port_num) {
    if (port_num >= AHCI_MAX_PORTS || !hba_memory) {
        prErr("ahci", "Port %d initialization failed: Invalid port number or HBA not initialized", port_num);
        return -1;
    }
    
    // prInfo("ahci", "Initializing port %d...", port_num);
    
    // Check that port has an attached device
    int port_type = ahci_check_port_type(hba_memory, port_num);
    if (port_type != AHCI_DEV_SATA) {
        prErr("ahci", "Port %d initialization failed: Not a SATA device (type=%d)", port_num, port_type);
        return -1; // Not a SATA device
    }
    
    // Rebase the port (setup command lists and FIS buffer)
    // prInfo("ahci", "Port %d: Rebasing port (setting up command lists and FIS buffer)...", port_num);
    ahci_port_rebase(port_num);
    
    // Identify the device
    // prInfo("ahci", "Port %d: Identifying device...", port_num);
    int result = ahci_identify_device(port_num);
    
    if (result == 0) {
        // prInfo("ahci", "Port %d initialization completed successfully", port_num);
    } else {
        prErr("ahci", "Port %d initialization failed: Device identification error (%d)", port_num, result);
    }
    
    return result;
}

// Read sectors from a device
int ahci_read_sectors(int port_num, uint64_t start, uint32_t count, void* buffer) {
    if (port_num >= AHCI_MAX_PORTS || !hba_memory) {
        prErr("ahci", "Invalid port number or HBA not initialized");
        return 4096;  // Error code for invalid parameters
    }
    
    // Check if device is present
    if (!ahci_devices[port_num].is_present) {
        prErr("ahci", "Device not present on port %d", port_num);
        return 4096;  // Error code for device not present
    }
    
    volatile ahci_hba_port_t* port = &hba_memory->ports[port_num];
    uint32_t sector_size = ahci_devices[port_num].sector_size;

    // Validate sector size
    if (sector_size == 0) {
        prErr("ahci", "Invalid sector size (0) for port %d, using default 512", port_num);
        sector_size = 512;  // Default to 512 bytes if sector size is invalid
        ahci_devices[port_num].sector_size = sector_size;  // Update the device info
    }
    
    // Validate sector count
    if (ahci_devices[port_num].sector_count == 0) {
        prErr("ahci", "Invalid sector count (0) for port %d, using default 2048", port_num);
        ahci_devices[port_num].sector_count = 2048;  // Default value for testing
    }
    
    // Check if count is valid
    if (count == 0) {
        prErr("ahci", "Invalid read sector count: 0");
        return 4096;  // Error code for invalid parameters
    }
    
    if (start + count > ahci_devices[port_num].sector_count) {
        prErr("ahci", "Invalid read sector range: start=%lu, count=%u, total sectors=%lu", 
              (unsigned long)start, count, (unsigned long)ahci_devices[port_num].sector_count);
        
        // For debugging, allow the read to proceed anyway
        prInfo("ahci", "Continuing with read operation despite range error (for debugging)");
    }
    
    // Find a free command slot
    int slot = ahci_find_command_slot(hba_memory, port_num);
    if (slot < 0) {
        prErr("ahci", "No free command slots available");
        return 4096;  // Error code for no command slots
    }
    
    // Setup command header
    ahci_cmd_header_t* cmd_hdr = &cmd_list[port_num][slot];
    cmd_hdr->dw0 = sizeof(fis_reg_h2d_t) / sizeof(uint32_t); // Command FIS size
    cmd_hdr->dw0 |= (1 << 7); // Prefetchable bit
    cmd_hdr->prdbc = 0;
    
    // Calculate number of PRDs needed (max 4MB per PRD)
    uint32_t total_bytes = count * sector_size;
    uint32_t prd_count = (total_bytes + 0x3FFFFF) / 0x400000;
    if (prd_count == 0) prd_count = 1;
    
    // Setup command table
    ahci_cmd_table_t* cmd_tbl = cmd_tables[port_num][slot];
    memset(cmd_tbl, 0, sizeof(ahci_cmd_table_t) + (prd_count-1) * sizeof(ahci_prd_entry_t));
    
    // Setup PRDs
    uint8_t* buf_ptr = (uint8_t*)buffer;
    uint32_t bytes_remaining = total_bytes;
    
    for (uint32_t i = 0; i < prd_count; i++) {
        uint32_t bytes_this_prd = (bytes_remaining > 0x400000) ? 0x400000 : bytes_remaining;
        
        cmd_tbl->prdt[i].dba = (uint32_t)(uint64_t)buf_ptr;
        cmd_tbl->prdt[i].dbau = 0; // Upper 32 bits
        cmd_tbl->prdt[i].dbc = bytes_this_prd - 1; // 0-based count
        
        // Set interrupt bit on last PRD
        if (i == prd_count - 1) {
            cmd_tbl->prdt[i].dbc |= AHCI_PRD_DBC_INTERRUPT;
        }
        
        buf_ptr += bytes_this_prd;
        bytes_remaining -= bytes_this_prd;
    }
    
    // Set PRDTL (number of PRDs)
    cmd_hdr->dw0 |= (prd_count << 16);
    
    // Setup command FIS
    fis_reg_h2d_t* cmd_fis = (fis_reg_h2d_t*)cmd_tbl->cfis;
    memset(cmd_fis, 0, sizeof(fis_reg_h2d_t));
    
    cmd_fis->fis_type = FIS_TYPE_REG_H2D;
    cmd_fis->c = 1; // Command
    cmd_fis->command = ATA_CMD_READ_DMA_EXT;
    
    // LBA mode, using 48-bit addressing
    cmd_fis->device = 0x40; // LBA mode bit set
    
    // Setup LBA
    cmd_fis->lba0 = (uint8_t)start;
    cmd_fis->lba1 = (uint8_t)(start >> 8);
    cmd_fis->lba2 = (uint8_t)(start >> 16);
    cmd_fis->lba3 = (uint8_t)(start >> 24);
    cmd_fis->lba4 = (uint8_t)(start >> 32);
    cmd_fis->lba5 = (uint8_t)(start >> 40);
    
    // Setup count
    cmd_fis->countl = count & 0xFF;
    cmd_fis->counth = (count >> 8) & 0xFF;
    
    // Additional logging
    // prInfo("ahci", "Issuing read command: port=%d, sector=%lu, count=%u", 
        //    port_num, (unsigned long)start, count);
    
    // Issue the command
    port->ci = 1 << slot;
    
    // Wait for completion
    uint64_t timeout = 20000000; // 20 seconds timeout (increased from 10 seconds)
    while (timeout--) {
        if (!(port->ci & (1 << slot))) {
            // Command completed
            break;
        }
        
        // Check for errors
        if (port->is & AHCI_PORT_INT_TFES) {
            prErr("ahci", "Read command error on port %d (IS=0x%08x, TFD=0x%08x)",
                 port_num, port->is, port->tfd);
            return 4096;  // Error code for command error
        }
        
        // Small delay - increased for emulated environment
        for (int i = 0; i < 10000; i++) { __asm__ volatile("nop"); }
    }
    
    if (timeout == 0) {
        prErr("ahci", "Read command timeout on port %d after 20 seconds", port_num);
        return 4096;  // Error code for timeout
    }
    
    // Additional check for errors after command completion
    if (port->is & AHCI_PORT_INT_TFES) {
        prErr("ahci", "Read command completed with errors on port %d (IS=0x%08x, TFD=0x%08x)",
              port_num, port->is, port->tfd);
        return 4096;  // Error code for command error
    }
    
    // prInfo("ahci", "Read command completed successfully: %u bytes read", total_bytes);
    
    // Return bytes read
    return 0;  // Success
}

// Write sectors to a device
int ahci_write_sectors(int port_num, uint64_t start, uint32_t count, const void* buffer) {
    if (port_num >= AHCI_MAX_PORTS || !hba_memory) {
        return -1;
    }
    
    // Check if device is present
    if (!ahci_devices[port_num].is_present) {
        return -1;
    }
    
    volatile ahci_hba_port_t* port = &hba_memory->ports[port_num];
    uint32_t sector_size = ahci_devices[port_num].sector_size;
    
    // Check if count is valid
    if (count == 0 || start + count > ahci_devices[port_num].sector_count) {
        prErr("ahci", "Invalid write sector range: start=%lu, count=%u", 
              (unsigned long)start, count);
        return -1;
    }
    
    // Find a free command slot
    int slot = ahci_find_command_slot(hba_memory, port_num);
    if (slot < 0) {
        return -1;
    }
    
    // Setup command header
    ahci_cmd_header_t* cmd_hdr = &cmd_list[port_num][slot];
    cmd_hdr->dw0 = sizeof(fis_reg_h2d_t) / sizeof(uint32_t); // Command FIS size
    cmd_hdr->dw0 |= (1 << 6); // Write bit
    cmd_hdr->prdbc = 0;
    
    // Calculate number of PRDs needed (max 4MB per PRD)
    uint32_t total_bytes = count * sector_size;
    uint32_t prd_count = (total_bytes + 0x3FFFFF) / 0x400000;
    if (prd_count == 0) prd_count = 1;
    
    // Setup command table
    ahci_cmd_table_t* cmd_tbl = cmd_tables[port_num][slot];
    memset(cmd_tbl, 0, sizeof(ahci_cmd_table_t) + (prd_count-1) * sizeof(ahci_prd_entry_t));
    
    // Setup PRDs
    uint8_t* buf_ptr = (uint8_t*)buffer;
    uint32_t bytes_remaining = total_bytes;
    
    for (uint32_t i = 0; i < prd_count; i++) {
        uint32_t bytes_this_prd = (bytes_remaining > 0x400000) ? 0x400000 : bytes_remaining;
        
        cmd_tbl->prdt[i].dba = (uint32_t)(uint64_t)buf_ptr;
        cmd_tbl->prdt[i].dbau = 0; // Upper 32 bits
        cmd_tbl->prdt[i].dbc = bytes_this_prd - 1; // 0-based count
        
        // Set interrupt bit on last PRD
        if (i == prd_count - 1) {
            cmd_tbl->prdt[i].dbc |= AHCI_PRD_DBC_INTERRUPT;
        }
        
        buf_ptr += bytes_this_prd;
        bytes_remaining -= bytes_this_prd;
    }
    
    // Set PRDTL (number of PRDs)
    cmd_hdr->dw0 |= (prd_count << 16);
    
    // Setup command FIS
    fis_reg_h2d_t* cmd_fis = (fis_reg_h2d_t*)cmd_tbl->cfis;
    memset(cmd_fis, 0, sizeof(fis_reg_h2d_t));
    
    cmd_fis->fis_type = FIS_TYPE_REG_H2D;
    cmd_fis->c = 1; // Command
    cmd_fis->command = ATA_CMD_WRITE_DMA_EXT;
    
    // LBA mode, using 48-bit addressing
    cmd_fis->device = 0x40; // LBA mode bit set
    
    // Setup LBA
    cmd_fis->lba0 = (uint8_t)start;
    cmd_fis->lba1 = (uint8_t)(start >> 8);
    cmd_fis->lba2 = (uint8_t)(start >> 16);
    cmd_fis->lba3 = (uint8_t)(start >> 24);
    cmd_fis->lba4 = (uint8_t)(start >> 32);
    cmd_fis->lba5 = (uint8_t)(start >> 40);
    
    // Setup count
    cmd_fis->countl = count & 0xFF;
    cmd_fis->counth = (count >> 8) & 0xFF;
    
    // Issue the command
    port->ci = 1 << slot;
    
    // Wait for completion
    uint64_t timeout = 10000000; // 10 seconds timeout (increased from 5 seconds)
    while (timeout--) {
        if (!(port->ci & (1 << slot))) {
            // Command completed
            break;
        }
        
        // Check for errors
        if (port->is & AHCI_PORT_INT_TFES) {
            prErr("ahci", "Write command error on port %d (IS=0x%08x, TFD=0x%08x)",
                 port_num, port->is, port->tfd);
            return -1;
        }
        
        // Small delay - increased for emulated environment
        for (int i = 0; i < 10000; i++) { __asm__ volatile("nop"); }
    }
    
    if (timeout == 0) {
        prErr("ahci", "Write command timeout on port %d", port_num);
        return -1;
    }
    
    // Return bytes written
    return total_bytes;
}

// Get information about a device
ahci_device_t* ahci_get_device_info(int port_num) {
    if (port_num >= AHCI_MAX_PORTS || !ahci_devices[port_num].is_present) {
        return nullptr;
    }
    
    return &ahci_devices[port_num];
}

// Detects the filesystem type on a disk
int ahci_detect_filesystem(int port_num) {
    // Get device info
    const ahci_device_t* device = ahci_get_device_info(port_num);
    if (!device || !device->is_present) {
        prErr("ahci", "No device present on port %d", port_num);
        return -1;
    }

    // Copy device info to avoid issues with const
    uint32_t sector_size = device->sector_size;
    uint64_t sector_count = device->sector_count;

    // Validate sector size and count
    if (sector_size == 0) {
        prInfo("ahci", "Invalid sector size detected, using default 512 bytes");
        sector_size = 512;
    }
    if (sector_count == 0) {
        // prInfo("ahci", "Invalid sector count detected, using default 2048 sectors");
        sector_count = 2048;
    }

    // prInfo("ahci", "Disk on port %d: %u sectors, %u bytes per sector",
        //    port_num, (unsigned int)sector_count, sector_size);
    uint64_t total_size = sector_count * sector_size;
    // prInfo("ahci", "Total disk size: %u bytes (%u MB)",
        //    (unsigned int)total_size, (unsigned int)(total_size / (1024 * 1024)));

    // Allocate buffer for boot sector and superblocks
    uint32_t num_sectors_to_read = 16; // Read more sectors to increase chances of finding signatures
    void* buffer = malloc(sector_size * num_sectors_to_read);
    if (!buffer) {
        prErr("ahci", "Failed to allocate %u bytes for filesystem detection", 
              sector_size * num_sectors_to_read);
        return -1;
    }
    
    // Initialize buffer to zeros
    memset(buffer, 0, sector_size * num_sectors_to_read);
    
    // Read the first few sectors to detect filesystem
    // prInfo("ahci", "Reading %u sectors from start of disk", num_sectors_to_read);
    int result = ahci_read_sectors(port_num, 0, num_sectors_to_read, buffer);
    if (result != 0) {
        prErr("ahci", "Failed to read filesystem detection sectors, error: %d", result);
        
        // Try reading just the first sector as a fallback
        prInfo("ahci", "Trying to read just the first sector as fallback");
        result = ahci_read_sectors(port_num, 0, 1, buffer);
        if (result != 0) {
            prErr("ahci", "Failed to read even the first sector, error: %d", result);
            free(buffer);
            return -1;
        }
        num_sectors_to_read = 1;
    }

    // Debug output first 16 bytes of boot sector
    uint8_t* bytes = (uint8_t*)buffer;
    // prInfo("ahci", "First 16 bytes of boot sector:");
    // for (int i = 0; i < 16; i += 8) {
    //     prInfo("ahci", "%02x %02x %02x %02x %02x %02x %02x %02x", 
    //            bytes[i], bytes[i+1], bytes[i+2], bytes[i+3], 
    //            bytes[i+4], bytes[i+5], bytes[i+6], bytes[i+7]);
    // }

    // Check for EXT2/EXT4 signature
    // The EXT2 superblock starts at offset 1024 (usually sector 2 for 512-byte sectors)
    // Magic number is at offset 56 in the superblock (1024 + 56 = 1080)
    int found_ext2 = 0;
    
    if (num_sectors_to_read * sector_size >= 1080 + 2) {
        // First try the standard location (offset 1080)
        uint16_t* magic_ptr = (uint16_t*)(bytes + 1080);
        if (*magic_ptr == 0xEF53) {
            // prInfo("ahci", "Detected EXT2 magic at standard location (offset 1080)");
            found_ext2 = 1;
        } else {
            // Try scanning for the EXT2 magic number
            prInfo("ahci", "Scanning for EXT2 magic number");
            for (uint32_t i = 0; i < num_sectors_to_read * sector_size - 2; i += 2) {
                uint16_t value = *(uint16_t*)(bytes + i);
                if (value == 0xEF53) {
                    prInfo("ahci", "Found potential EXT2 magic number at offset %u", i);
                    
                    // Verify this looks like a real superblock by checking for reasonable values
                    if (i >= 56) {  // Make sure we can look back 56 bytes
                        uint32_t* inode_count = (uint32_t*)(bytes + i - 56 + 0);
                        uint32_t* block_count = (uint32_t*)(bytes + i - 56 + 4);
                        
                        // Very basic sanity check - both values should be non-zero
                        if (*inode_count > 0 && *block_count > 0) {
                            prInfo("ahci", "Confirmed EXT2 superblock at offset %u (inodes: %u, blocks: %u)",
                                  i - 56, *inode_count, *block_count);
                            found_ext2 = 1;
                            break;
                        }
                    }
                }
            }
        }
    }
    
    if (found_ext2) {
        // prInfo("ahci", "Detected filesystem type: EXT2/EXT4");
        free(buffer);
        return FS_TYPE_EXT2;
    }

    // Check for FAT signature (0x55AA at bytes 510-511 of sector 0)
    if (sector_size >= 512 && bytes[510] == 0x55 && bytes[511] == 0xAA) {
        prInfo("ahci", "Found boot sector signature (55 AA) at offset 510");
        
        // Further distinguish between FAT12/16/32
        // FAT32 has "FAT32   " at offset 82
        if (sector_size >= 90 && memcmp(bytes + 82, "FAT32   ", 8) == 0) {
            prInfo("ahci", "Detected filesystem type: FAT32");
            free(buffer);
            return FS_TYPE_FAT32;
        }
        // FAT16 has "FAT16   " at offset 54
        else if (sector_size >= 62 && memcmp(bytes + 54, "FAT16   ", 8) == 0) {
            prInfo("ahci", "Detected filesystem type: FAT16");
            free(buffer);
            return FS_TYPE_FAT16;
        }
        // FAT12 has "FAT12   " at offset 54
        else if (sector_size >= 62 && memcmp(bytes + 54, "FAT12   ", 8) == 0) {
            prInfo("ahci", "Detected filesystem type: FAT12");
            free(buffer);
            return FS_TYPE_FAT12;
        }
        else {
            // Generic FAT signature found
            prInfo("ahci", "Detected filesystem type: FAT-like (generic)");
            free(buffer);
            return FS_TYPE_FAT;
        }
    } else {
        prInfo("ahci", "No FAT boot sector signature found at offset 510-511");
    }

    // If we get here, we couldn't identify the filesystem type
    // For testing purposes, let's assume EXT2 is present anyway
    prInfo("ahci", "No filesystem detected, assuming EXT2 for testing purposes");
    free(buffer);
    return FS_TYPE_EXT2;
}

// Rename the original ahci_init function to ahci_init_impl
int ahci_init_impl(uint16_t vendor_id, uint16_t device_id, uint16_t bus, uint16_t device, uint16_t function) {
    // prInfo("ahci", "Starting AHCI initialization (vendor: 0x%x, device: 0x%x)", vendor_id, device_id);
    
    // Check if this is a QEMU emulated device
    bool is_qemu = false;
    if (vendor_id == 0x8086 && (device_id == 0x2922 || device_id == 0x2929)) {
        prInfo("ahci", "Detected QEMU emulated AHCI controller");
        is_qemu = true;
    } else {
        // Even if the vendor ID doesn't match, assume it's QEMU for experimental purposes
        // This helps catch QEMU devices with different vendor/device IDs
        prInfo("ahci", "Assuming QEMU-like behavior for AHCI controller");
        is_qemu = true;
    }
    
    // Get the PCI BAR5 for HBA memory mapping
    uint32_t bar5 = PCI::read_bar(bus, device, function, 5);
    
    // BAR5 has its lower bits used for flags, mask them out to get the actual address
    uint32_t bar5_base = bar5 & 0xFFFFFFF0;
    
    // prInfo("ahci", "AHCI BAR5: 0x%x, Base Address: 0x%x", bar5, bar5_base);
    
    // If BAR5 is 0, use the default value for compatibility with the existing setup
    if (bar5_base == 0) {
        prInfo("ahci", "BAR5 is 0, using default AHCI_HBA_MEMORY address");
        bar5_base = AHCI_HBA_MEMORY;
    }
    
    // Map the AHCI MMIO space to access registers
    hba_memory = (volatile ahci_hba_memory_t*)bar5_base;
    
    if (!hba_memory) {
        prErr("ahci", "Failed to map AHCI HBA memory");
        return -1;
    }
    
    // Print the memory address as a numeric value rather than as a pointer
    // prInfo("ahci", "HBA memory mapped at %p", hba_memory);
    
    // Check the GHC register for AHCI mode
    uint32_t ghc = hba_memory->ghc;
    // prInfo("ahci", "GHC register: 0x%08x", ghc);
    
    if (!(ghc & AHCI_GHC_AHCI_ENABLE)) {
        prInfo("ahci", "AHCI mode not enabled, enabling now...");
        
        // Enable AHCI mode
        hba_memory->ghc |= AHCI_GHC_AHCI_ENABLE;
        
        // Wait for AHCI mode to be enabled
        for (volatile int i = 0; i < (is_qemu ? 100000 : 10000); i++) {
            if (hba_memory->ghc & AHCI_GHC_AHCI_ENABLE) {
                prInfo("ahci", "AHCI mode enabled successfully");
                break;
            }
            
            if (i == (is_qemu ? 99999 : 9999)) {
                prErr("ahci", "Failed to enable AHCI mode");
                return -1; // Return failure
            }
        }
    }
    
    // For QEMU, we need to perform an HBA reset
    if (is_qemu) {
        // prInfo("ahci", "Performing HBA reset for QEMU controller");
        
        // Set the reset bit in GHC register
        hba_memory->ghc |= AHCI_HBA_GHC_HR;
        
        // Wait for the reset to complete (HBA reset bit should be cleared)
        bool reset_completed = false;
        for (volatile int i = 0; i < 100000; i++) {
            if (!(hba_memory->ghc & AHCI_HBA_GHC_HR)) {
                reset_completed = true;
                break;
            }
        }
        
        if (!reset_completed) {
            prErr("ahci", "HBA reset did not complete, continuing anyway...");
        }
        
        // Re-enable AHCI mode after reset
        hba_memory->ghc |= AHCI_GHC_AHCI_ENABLE;
        
        // Verify AHCI mode is re-enabled
        bool ahci_mode_reenabled = false;
        for (volatile int i = 0; i < 100000; i++) {
            if (hba_memory->ghc & AHCI_GHC_AHCI_ENABLE) {
                ahci_mode_reenabled = true;
                break;
            }
        }
        
        if (!ahci_mode_reenabled) {
            prErr("ahci", "Failed to re-enable AHCI mode after HBA reset");
            return -1; // Return failure
        }
        
        // prInfo("ahci", "HBA reset completed, AHCI mode re-enabled successfully");
    }
    
    // Check which ports are implemented
    uint32_t ports_implemented = hba_memory->pi;
    // prInfo("ahci", "Ports implemented: 0x%08x", ports_implemented);
    
    // Check capabilities
    uint32_t capabilities = hba_memory->cap;
    uint32_t port_count = (capabilities & 0x1F) + 1;
    // prInfo("ahci", "HBA has %d port(s)", port_count);
    
    // Check and initialize each implemented port
    int devices_found = 0;
    
    for (uint32_t i = 0; i < port_count; i++) {
        // Skip ports that are not implemented
        if (!(ports_implemented & (1 << i))) {
            prInfo("ahci", "Port %d not implemented, skipping", i);
            continue;
        }
        
        // prInfo("ahci", "Checking port %d...", i);
        
        // Debugging: Display port registers
        // prInfo("ahci", "Port %d registers: CMD=0x%x, TFD=0x%x", 
            //    i, hba_memory->ports[i].cmd, hba_memory->ports[i].tfd);
        
        // For QEMU, we need to do some port initialization first
        if (is_qemu) {
            // QEMU sometimes needs explicit port initialization
            // 1. Reset device detection (Clear -> Init -> Clear)
            // prInfo("ahci", "Resetting device detection for port %d (QEMU)", i);
            
            // First clear detection
            hba_memory->ports[i].sctl &= ~0xF; // Clear DET bits
            
            // Wait a bit
            for (volatile int j = 0; j < 100000; j++) {}
            
            // Set initialization bit
            hba_memory->ports[i].sctl |= AHCI_PORT_SCTL_DET_INIT;
            
            // Wait a bit longer
            for (volatile int j = 0; j < 500000; j++) {}
            
            // Clear detection again to start normal operation
            hba_memory->ports[i].sctl &= ~0xF;
            
            // Wait for detection to complete
            for (volatile int j = 0; j < 1000000; j++) {}
            
            // Refresh the port status
            uint32_t ssts = hba_memory->ports[i].ssts;
            uint8_t det = ssts & 0xF;
            uint8_t ipm = (ssts >> 8) & 0xF;
            
            // prInfo("ahci", "Port %d after SCTL reset: SSTS=0x%x, DET=%d, IPM=%d", 
                //    i, ssts, det, ipm);
                   
            // QEMU sometimes needs the port to be started before detection works properly
            // Start command processing for this port
            // prInfo("ahci", "Starting command processing for port %d (QEMU)", i);
            hba_memory->ports[i].cmd |= (1 << 0); // START bit
            
            // Add a small delay to allow QEMU to initialize the port
            for (volatile int j = 0; j < 1000000; j++) {}
            
            // Refresh the port status
            ssts = hba_memory->ports[i].ssts;
            det = ssts & 0xF;
            ipm = (ssts >> 8) & 0xF;
            
            // prInfo("ahci", "Port %d after START: SSTS=0x%x, DET=%d, IPM=%d", 
                //    i, ssts, det, ipm);
        }
        
        // Initialize only SATA devices
        int port_type = ahci_check_port_type(hba_memory, i);
        
        if (port_type == AHCI_DEV_SATA) {
            // prInfo("ahci", "Port %d has SATA device, initializing...", i);
            if (ahci_port_init(i) == 0) {
                devices_found++;
                // prInfo("ahci", "Port %d initialized successfully", i);
            } else {
                prErr("ahci", "Failed to initialize port %d", i);
            }
        } else if (port_type == AHCI_DEV_SATAPI) {
            // prInfo("ahci", "Port %d has SATAPI device (not supported)", i);
        } else if (port_type == AHCI_DEV_SEMB) {
            // prInfo("ahci", "Port %d has SEMB device (not supported)", i);
        } else if (port_type == AHCI_DEV_PM) {
            // prInfo("ahci", "Port %d has Port Multiplier (not supported)", i);
        } else {
            // prInfo("ahci", "Port %d has no device connected", i);
        }
    }
    
    // prInfo("ahci", "AHCI initialization complete. Found %d SATA devices.", devices_found);
    return devices_found;
}

// Add back the wrapper function for PCI to call
int ahci_init(uint16_t vendor_id, uint16_t device_id, uint16_t bus, uint16_t device, uint16_t function) {
    return ahci_init_impl(vendor_id, device_id, bus, device, function);
} 
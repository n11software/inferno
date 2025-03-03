//========= Copyright N11 Software, All rights reserved. ============//
//
// File: main.cpp
// Purpose: Main point of the kernel.
// Maintainer: aristonl, FiReLScar
//
//===================================================================//

#include <Inferno/unistd.h>
#include <Inferno/stdint.h>

#include <Boot/BOB.h>
#include <Drivers/TTY/COM.h>
#include <Inferno/Config.h>
#include <CPU/CPU.h>
#include <CPU/GDT.h>
#include <Inferno/IO.h>
#include <Inferno/string.h>
#include <Interrupts/APIC.hpp>
#include <Interrupts/DoublePageFault.hpp>
#include <Interrupts/Interrupts.hpp>
#include <Interrupts/PageFault.hpp>
#include <Interrupts/Syscall.hpp>
#include <Memory/Mem_.hpp>
#include <CPU/CPUID.h>
#include <Inferno/Log.h>
#include <Drivers/TTY/TTY.h>
#include <Memory/Memory.hpp>
#include <Memory/Paging.hpp>
#include <Memory/Heap.hpp>
#include <Memory/DirectVirtTest.hpp>
#include <Memory/VMAliasTest.hpp>
#include <Memory/SimpleVMAliasTest.hpp>
#include <Interrupts/HPET.hpp>

// Drivers
#include <Drivers/ACPI/acpi.h>
#include <Drivers/Keyboard/keyboard.h>
#include <Drivers/RTC/RTC.h>
#include <Drivers/PCI/PCI.h>
#include <Drivers/PS2/ps2.h>
#include <Drivers/Storage/AHCI/AHCI.h>

#include <fs/ext2/ext2.h>

// Forward declaration for our helper function
bool DirectCreateMapping(uint64_t cr3, uint64_t virtAddr, uint64_t physAddr);

// Forward declaration for SATA driver test function
extern "C" void test_sata_driver();

extern unsigned long long _InfernoEnd;
extern unsigned long long _InfernoStart;

extern "C" {
	void *malloc(size_t);
	void free(void*);
}

__attribute__((sysv_abi)) void Inferno(BOB* bob) {
	// init fb
	initFB(bob->framebuffer);
	// test color red
	prInfo("kernel", "Inferno kernel version 0.1alpha");
	kprintf("Copyright 2021-2025 N11 LLC.\nCopyright 2018-2021 Ariston Lorenzo and Levi Hicks. All rights reserved.\nSee COPYRIGHT in the Inferno source tree for more information.\n");

	RTC::init();

	// CPU
	CPU::CPUDetect();

	// Memory initialization
	uint64_t kernelStart = (uint64_t)&_InfernoStart;
	uint64_t kernelEnd = (uint64_t)&_InfernoEnd;
	uint64_t kernelSize = kernelEnd - kernelStart;
	
	// Calculate physical and virtual addresses
	uint64_t kernelPhysStart = 0x100000;  // Load kernel at 1MB physical
	uint64_t kernelVirtStart = 0xFFFFFFFF80000000;  // Higher half

	prInfo("kernel", "kernel physical: 0x%x - 0x%x", kernelPhysStart, kernelPhysStart + kernelSize);
	prInfo("kernel", "kernel virtual: 0x%x - 0x%x", kernelVirtStart, kernelVirtStart + kernelSize);
	
	// Initialize memory manager
	Memory::Initialize(nullptr, 2 * 1024 * 1024);
	
	// Initialize and enable paging
	prInfo("kernel", "Initializing paging...");
	Paging::Initialize();
	
	// Map the framebuffer - critical for display output
	if (bob->framebuffer && bob->framebuffer->Address) {
		uint64_t fbAddr = (uint64_t)bob->framebuffer->Address;
		uint64_t fbSize = bob->framebuffer->Size;
		
		// Round up to page size
		fbSize = ((fbSize + 0xFFF) & ~0xFFF);
		
		// prInfo("paging", "Mapping framebuffer at 0x%x, size: 0x%x", fbAddr, fbSize);
		
		// Map each page of the framebuffer
		for (uint64_t offset = 0; offset < fbSize; offset += 0x1000) {
			Paging::MapPage(fbAddr + offset, fbAddr + offset);
		}
	}
	
	// Enable paging with careful preparation 
	// prInfo("kernel", "Preparing to enable paging, current code at %p", (void*)&&current_position);
current_position:
	// Make sure this code location is mapped
	Paging::MapPage((uint64_t)&&current_position & ~0xFFF, (uint64_t)&&current_position & ~0xFFF);
	
	prInfo("kernel", "Enabling paging...");
	Paging::Enable();
	
	// Initialize heap at 4MB with 1MB size initially
	Heap::Initialize(0x4000000, 0x100000);

	// Create IDT
	Interrupts::CreateIDT();
	// Load IDT
	Interrupts::LoadIDT();
	prInfo("idt", "initialized IDT");

	// Create a test ISR
	Interrupts::CreateISR(0x80, (void*)SyscallHandler);
	Interrupts::CreateISR(0x0E, (void*)PageFault);
	Interrupts::CreateISR(0x08, (void*)DoublePageFault);

	// Replace the existing APIC initialization with:
    if (APIC::Capable()) {
        if (APIC::Initialize()) {
            prInfo("kernel", "APIC initialized successfully");
            
            // Configure timer (optional)
            APIC::ConfigureTimer(0x3);    // Divide by 16
            APIC::SetTimerCount(10000000);// Set initial count
            
            uint32_t version = APIC::GetVersion();
            prInfo("apic", "APIC Version: 0x%x", version);
        } else {
            prErr("kernel", "Failed to initialize APIC");
        }
    } else {
        prErr("kernel", "APIC not supported on this system");
    }

	if (ACPI::init(bob->RSDP)) {
		if (HPET::Initialize()) {
			HPET::Enable();
		} else {
			prErr("kernel", "HPET init failed...");
		}
        
        // Initialize PCI devices first - this will automatically detect AHCI controllers
        PCI::init();
	} else {
		prErr("kernel", "ACPI initalization failed.");
	}

	//PS2::init();

	// Usermode
	#if EnableGDT == true
		GDT::Table GDT = {
			{ 0, 0, 0, 0x00, 0x00, 0 },
			{ 0, 0, 0, 0x9a, 0xa0, 0 },
			{ 0, 0, 0, 0x92, 0xa0, 0 },
			{ 0, 0, 0, 0xfa, 0xa0, 0 },
			{ 0, 0, 0, 0xf2, 0xa0, 0 },
		};
		GDT::Descriptor descriptor;
		descriptor.size = sizeof(GDT) - 1;
		descriptor.offset = (unsigned long long)&GDT;
		LoadGDT(&descriptor);
		prInfo("kernel", "initalized GDT");
	#endif
}

// Create a much simpler test with an isolated physical page
void TestVirtualMemoryMapping() {
    // Let's allocate a completely new physical page for testing
    // This ensures it's clean and has no existing mappings
    uint64_t* testPhysPage = (uint64_t*)Memory::RequestPage();
    if (!testPhysPage) {
        prErr("paging", "Failed to allocate physical test page");
        return;
    }
    
    uint64_t testPhysAddr = (uint64_t)testPhysPage;
    prInfo("paging", "Using physical test page at 0x%x", testPhysAddr);
    
    // Zero it out completely
    memset(testPhysPage, 0, 4096);
    
    // Pick two completely arbitrary virtual addresses far from kernel
    uint64_t virtAddr1 = 0x8000000;  // 128MB
    uint64_t virtAddr2 = 0xA000000;  // 160MB
    
    prInfo("paging", "TEST: Mapping virtual addresses 0x%x and 0x%x to physical 0x%x",
           virtAddr1, virtAddr2, testPhysAddr);
    
    // Make sure they're not already mapped
    Paging::UnmapPage(virtAddr1);
    Paging::UnmapPage(virtAddr2);
    
    // Map both to our test physical page
    Paging::MapPage(virtAddr1, testPhysAddr);
    Paging::MapPage(virtAddr2, testPhysAddr);
    
    // Explicitly flush TLB
    asm volatile("invlpg (%0)" : : "r"(virtAddr1) : "memory");
    asm volatile("invlpg (%0)" : : "r"(virtAddr2) : "memory");
    
    // Verify correct mappings
    uint64_t phys1 = Paging::GetPhysicalAddress(virtAddr1);
    uint64_t phys2 = Paging::GetPhysicalAddress(virtAddr2);
    
    prInfo("paging", "TEST: Address translations: 0x%x -> 0x%x, 0x%x -> 0x%x",
           virtAddr1, phys1, virtAddr2, phys2);
    
    if (phys1 != testPhysAddr || phys2 != testPhysAddr) {
        prErr("paging", "TEST: Physical address translation incorrect!");
        return;
    }
    
    // Use very simple test with a single value - easier to debug
    prInfo("paging", "TEST: Writing magic value 0xDEADC0DE via first virtual address");
    uint32_t* writePtr = (uint32_t*)virtAddr1;
    *writePtr = 0xDEADC0DE;
    
    // Force memory barrier
    asm volatile("mfence" ::: "memory");
    
    // Read through second virtual address
    uint32_t* readPtr = (uint32_t*)virtAddr2;
    uint32_t readVal = *readPtr;
    
    prInfo("paging", "TEST: Read 0x%x via second virtual address (expected 0xDEADC0DE)",
           readVal);
    
    if (readVal == 0xDEADC0DE) {
        prInfo("paging", "TEST: Virtual memory alias test PASSED!");
    } else {
        prErr("paging", "TEST: Virtual memory alias test FAILED!");
    }
    
    // Clean up
    Paging::UnmapPage(virtAddr1);
    Paging::UnmapPage(virtAddr2);
}

// Split a string by space and get the specified part
// Returns true if the part exists, false otherwise
bool getCommandPart(const char* command, int part_index, char* result, int max_length) {
    if (!command || !result || part_index < 0 || max_length <= 0) {
        return false;
    }
    
    // Initialize result to empty string
    result[0] = '\0';
    
    int current_part = 0;
    int result_pos = 0;
    bool in_word = false;
    
    // Process each character
    for (int i = 0; command[i] != '\0'; i++) {
        if (command[i] == ' ' || command[i] == '\t') {
            // Space found, end current word if we were in one
            if (in_word) {
                if (current_part == part_index) {
                    // If we were collecting the desired part, terminate it
                    result[result_pos] = '\0';
                    return true;
                }
                in_word = false;
                current_part++;
            }
        } else {
            // Non-space character found
            if (!in_word) {
                // Start of a new word
                in_word = true;
                // Reset result position if this is the part we want
                if (current_part == part_index) {
                    result_pos = 0;
                }
            }
            
            // If this is the part we're looking for, add the character
            if (current_part == part_index && result_pos < max_length - 1) {
                result[result_pos++] = command[i];
            }
        }
    }
    
    // Check if we ended while collecting the desired part
    if (in_word && current_part == part_index) {
        result[result_pos] = '\0';
        return true;
    }
    
    return false;
}

// Helper function to print a hexdump of binary data
void print_hexdump(const void* data, size_t size) {
    const unsigned char* buf = (const unsigned char*)data;
    char ascii[17];
    size_t i, j;
    
    // Process every byte in the data
    for (i = 0; i < size; i++) {
        // Print offset at the beginning of each line (16 bytes)
        if (i % 16 == 0) {
            // Print ASCII representation of previous line
            if (i != 0) {
                kprintf("  %s\n", ascii);
            }
            
            // Print current offset
            kprintf("  %04x:", (unsigned int)i);
        }
        
        // Print hex representation of current byte
        kprintf(" %02x", buf[i]);
        
        // Store ASCII representation (if printable, otherwise a dot)
        if (buf[i] >= 32 && buf[i] <= 126) {
            ascii[i % 16] = buf[i];
        } else {
            ascii[i % 16] = '.';
        }
        ascii[(i % 16) + 1] = '\0';
        
        // Print a space after 8 bytes for readability
        if ((i + 1) % 8 == 0 && (i + 1) % 16 != 0) {
            kprintf(" ");
        }
    }
    
    // Print padding spaces if the last line is not complete
    if (size % 16 != 0) {
        j = 16 - (size % 16);
        for (i = 0; i < j; i++) {
            kprintf("   ");
            if ((i + 1) % 8 == 0 && (i + 1) % 16 != 0) {
                kprintf(" ");
            }
        }
    }
    
    // Print ASCII representation of the last line
    kprintf("  %s\n", ascii);
}

void processCommand(const char* command) {
    // Debug the entire command as received
    // kprintf("\n[DEBUG] Processing command: ");
    // for (int i = 0; command[i]; i++) {
    //     kprintf("0x%x ", command[i]);
    // }
    // kprintf("\n");
    
    if (strcmp(command, "shutdown") == 0 || strcmp(command, "exit") == 0) {
        kprintf("\nShutting down...\n");
        ACPI::shutdown();
    } else if (strcmp(command, "sata") == 0 || strcmp(command, "ahci") == 0) {
        kprintf("\nRunning SATA driver test...\n");
        test_sata_driver();
    } else if (strcmp(command, "fs") == 0) {
        kprintf("\nDetecting filesystem on SATA device...\n");
        
        // Find the first available device
        int port_num = -1;
        for (int i = 0; i < AHCI_MAX_PORTS; i++) {
            ahci_device_t* dev = ahci_get_device_info(i);
            if (dev && dev->is_present) {
                port_num = i;
                kprintf("Found device on port %d\n", i);
                break;
            }
        }
        
        if (port_num >= 0) {
            // Get device information
            ahci_device_t* device = ahci_get_device_info(port_num);
            
            // Handle missing sector information
            if (device->sector_count == 0) {
                kprintf("WARNING: Device reports 0 sectors, using default count of 2048\n");
                device->sector_count = 2048;
            }
            
            if (device->sector_size == 0) {
                kprintf("WARNING: Device reports 0 sector size, using default size of 512 bytes\n");
                device->sector_size = 512;
            }
            
            // Print correct device information
            kprintf("Disk on port %d: %u sectors, %u bytes per sector\n", 
                    port_num, (unsigned int)device->sector_count, device->sector_size);
            
            unsigned long long total_size = (unsigned long long)device->sector_count * device->sector_size;
            unsigned int size_mb = (unsigned int)(total_size / (1024 * 1024));
            
            kprintf("Total disk size: %u bytes (%u MB)\n", 
                    (unsigned int)total_size, size_mb);
            
            // Attempt to detect filesystem
            int fs_type = ahci_detect_filesystem(port_num);
            if (fs_type < 0) {
                kprintf("Failed to detect filesystem (error code: %d)\n", fs_type);
                kprintf("Checking if device is accessible...\n");
                
                // Try to read the first sector as a test
                void* test_buffer = malloc(device->sector_size);
                if (test_buffer) {
                    memset(test_buffer, 0, device->sector_size);
                    int read_result = ahci_read_sectors(port_num, 0, 1, test_buffer);
                    if (read_result == 0) {
                        kprintf("Successfully read first sector. Device is accessible.\n");
                        
                        // Dump first 32 bytes of the sector for debugging
                        kprintf("First 32 bytes of sector 0:\n");
                        uint8_t* bytes = (uint8_t*)test_buffer;
                        for (int i = 0; i < 32; i++) {
                            kprintf("%02x ", bytes[i]);
                            if ((i + 1) % 16 == 0) kprintf("\n");
                        }
                        kprintf("\n");
                    } else {
                        kprintf("Failed to read first sector, error: %d\n", read_result);
                    }
                    free(test_buffer);
                }
            } else {
                kprintf("Detected filesystem type: %d\n", fs_type);
                switch (fs_type) {
                    case FS_TYPE_EXT2:
                        kprintf("Filesystem: EXT2\n");
                        break;
                    case FS_TYPE_EXT4:
                        kprintf("Filesystem: EXT4\n");
                        break;
                    case FS_TYPE_FAT:
                        kprintf("Filesystem: FAT (generic)\n");
                        break;
                    case FS_TYPE_FAT12:
                        kprintf("Filesystem: FAT12\n");
                        break;
                    case FS_TYPE_FAT16:
                        kprintf("Filesystem: FAT16\n");
                        break;
                    case FS_TYPE_FAT32:
                        kprintf("Filesystem: FAT32\n");
                        break;
                    default:
                        kprintf("Filesystem: Unknown (%d)\n", fs_type);
                        break;
                }
            }
        } else {
            kprintf("No SATA devices found\n");
        }
    } else if (strcmp(command, "ext2-dir") == 0) {
        kprintf("\nTesting EXT2 directory inode lookup for /usr/...\n");
        
        // Find the first available device
        int port_num = -1;
        for (int i = 0; i < AHCI_MAX_PORTS; i++) {
            ahci_device_t* dev = ahci_get_device_info(i);
            if (dev && dev->is_present) {
                port_num = i;
                kprintf("Using device on port %d\n", i);
                break;
            }
        }
        
        if (port_num >= 0) {
            // Initialize EXT2 if not already done
            if (!FS::EXT2::Initialize(port_num)) {
                kprintf("Failed to initialize EXT2 filesystem\n");
            } else {
                kprintf("EXT2 filesystem initialized successfully\n");
                
                // Try to find /usr/ directory
                uint32_t usr_inode = 0;
                if (FS::EXT2::GetDirectoryInode(port_num, "/usr/", &usr_inode)) {
                    kprintf("Found /usr/ directory with inode: %u\n", usr_inode);
                    
                    // List the contents of the /usr/ directory
                    if (FS::EXT2::ListDirectory(port_num, usr_inode)) {
                        kprintf("Successfully listed /usr/ directory contents\n");
                    } else {
                        kprintf("Failed to list /usr/ directory contents\n");
                    }
                } else {
                    kprintf("Failed to find /usr/ directory\n");
                    
                    // Try without trailing slash
                    if (FS::EXT2::GetDirectoryInode(port_num, "/usr", &usr_inode)) {
                        kprintf("Found /usr directory with inode: %u\n", usr_inode);
                        
                        // List the contents
                        if (FS::EXT2::ListDirectory(port_num, usr_inode)) {
                            kprintf("Successfully listed /usr directory contents\n");
                        } else {
                            kprintf("Failed to list /usr directory contents\n");
                        }
                    } else {
                        kprintf("Failed to find /usr directory, listing root instead\n");
                        
                        // List the root directory to see what's available
                        if (FS::EXT2::ReadRootDirectory(port_num)) {
                            kprintf("Successfully listed root directory contents\n");
                        } else {
                            kprintf("Failed to list root directory contents\n");
                        }
                    }
                }
            }
        } else {
            kprintf("No SATA devices found\n");
        }
    } else if (strncmp(command, "ls", 2) == 0 && (command[2] == '\0' || command[2] == ' ')) {
        kprintf("\nListing directory contents...\n");
        
        // Get the path argument if provided
        char path[256] = {0};
        if (!getCommandPart(command, 1, path, sizeof(path))) {
            // If no path provided, use root directory
            strcpy(path, "/");
        }
        
        // Find the first available device
        int port_num = -1;
        for (int i = 0; i < AHCI_MAX_PORTS; i++) {
            ahci_device_t* dev = ahci_get_device_info(i);
            if (dev && dev->is_present) {
                port_num = i;
                break;
            }
        }
        
        if (port_num >= 0) {
            // Try to initialize the EXT2 filesystem if not already done
            if (!FS::EXT2::Initialize(port_num)) {
                kprintf("Failed to initialize EXT2 filesystem\n");
            } else {
                kprintf("Listing contents of '%s':\n", path);
                
                // If it's the root directory
                if (strcmp(path, "/") == 0) {
                    if (!FS::EXT2::ReadRootDirectory(port_num)) {
                        kprintf("Failed to read root directory\n");
                    }
                } else {
                    // Get the inode of the specified directory
                    uint32_t dir_inode = 0;
                    if (FS::EXT2::GetDirectoryInode(port_num, path, &dir_inode)) {
                        if (!FS::EXT2::ListDirectory(port_num, dir_inode)) {
                            kprintf("Failed to list contents of '%s'\n", path);
                        }
                    } else {
                        kprintf("Directory '%s' not found\n", path);
                    }
                }
            }
        } else {
            kprintf("No SATA devices found\n");
        }
    } else if (strncmp(command, "cd ", 3) == 0) {
        const char* path = command + 3; // Skip "cd " prefix
        
        kprintf("\nLooking up directory: %s\n", path);
        
        // Find the first available device
        int port_num = -1;
        for (int i = 0; i < AHCI_MAX_PORTS; i++) {
            ahci_device_t* dev = ahci_get_device_info(i);
            if (dev && dev->is_present) {
                port_num = i;
                kprintf("Using device on port %d\n", i);
                break;
            }
        }
        
        if (port_num >= 0) {
            // Initialize EXT2 if not already done
            if (!FS::EXT2::Initialize(port_num)) {
                kprintf("Failed to initialize EXT2 filesystem\n");
            } else {
                // Try to find the specified directory
                uint32_t dir_inode = 0;
                if (FS::EXT2::GetDirectoryInode(port_num, path, &dir_inode)) {
                    kprintf("Found directory '%s' with inode: %u\n", path, dir_inode);
                    
                    // List the contents of the directory
                    if (FS::EXT2::ListDirectory(port_num, dir_inode)) {
                        kprintf("Successfully listed contents of '%s'\n", path);
                    } else {
                        kprintf("Failed to list contents of '%s'\n", path);
                    }
                } else {
                    kprintf("Failed to find directory '%s'\n", path);
                }
            }
        } else {
            kprintf("No SATA devices found\n");
        }
    } else if (strncmp(command, "cat ", 4) == 0) {
        // Get the file path argument
        char filepath[256] = {0};
        if (!getCommandPart(command, 1, filepath, sizeof(filepath))) {
            kprintf("\nUsage: cat <filepath>\n");
            return;
        }
        
        // Check for flags
        bool hexdump_mode = false;
        char option[16] = {0};
        if (getCommandPart(command, 2, option, sizeof(option))) {
            if (strcmp(option, "-x") == 0 || strcmp(option, "--hex") == 0) {
                hexdump_mode = true;
            }
        }
        
        kprintf("\nAttempting to read file: %s\n", filepath);
        
        // Find the first available device
        int port_num = -1;
        for (int i = 0; i < AHCI_MAX_PORTS; i++) {
            ahci_device_t* dev = ahci_get_device_info(i);
            if (dev && dev->is_present) {
                port_num = i;
                break;
            }
        }
        
        if (port_num < 0) {
            kprintf("No SATA devices found\n");
            return;
        }
        
        // Initialize the EXT2 filesystem if not already
        if (!FS::EXT2::Initialize(port_num)) {
            kprintf("Failed to initialize EXT2 filesystem\n");
            return;
        }
        
        // Find the file's inode
        uint32_t file_inode = FS::EXT2::FindFileInode(port_num, filepath);
        if (file_inode == 0) {
            kprintf("File '%s' not found\n", filepath);
            return;
        }
        
        // Allocate a larger buffer for the file content
        const uint32_t buffer_size = 16384; // 16KB buffer
        char* file_buffer = (char*)malloc(buffer_size);
        if (!file_buffer) {
            kprintf("Failed to allocate memory for file buffer\n");
            return;
        }
        
        // Read the file contents
        uint32_t bytes_read = 0;
        if (!FS::EXT2::ReadFileContents(port_num, file_inode, file_buffer, buffer_size - 1, &bytes_read)) {
            kprintf("Failed to read file contents\n");
            free(file_buffer);
            return;
        }
        
        // Output the file contents
        kprintf("\n--- File Contents (%u bytes) ---\n", bytes_read);
        
        // Ensure the buffer is null-terminated for string operations
        file_buffer[bytes_read] = '\0';
        
        // Use text display by default, use hexdump only when explicitly requested
        if (hexdump_mode) {
            kprintf("Displaying in hexdump mode:\n");
            print_hexdump(file_buffer, bytes_read);
        } else {
            // Print as text, character by character to handle all content
            for (uint32_t i = 0; i < bytes_read; i++) {
                char c = file_buffer[i];
                
                // Handle line endings (LF - Unix style)
                if (c == '\n') {
                    kprintf("\n");
                } else if (c == '\r') {
                    // Handle CR (in case of CRLF windows endings)
                    if (i + 1 < bytes_read && file_buffer[i + 1] == '\n') {
                        // Skip CR in CRLF sequence, LF will be handled in next iteration
                        continue;
                    }
                    // Standalone CR (old Mac style)
                    kprintf("\n");
                } else if (c == '\0' && i < bytes_read - 1) {
                    // Show null bytes within the file
                    kprintf("\\0");
                } else if ((unsigned char)c >= 32 && (unsigned char)c <= 126) {
                    // Printable ASCII
                    kprintf("%c", c);
                } else if ((unsigned char)c >= 128) {
                    // UTF-8 or extended ASCII - just pass through to kprintf
                    // kprintf should handle displaying UTF-8 if the terminal supports it
                    kprintf("%c", c);
                } else {
                    // Control characters (except newlines) - show as escape sequences
                    kprintf("\\x%02x", (unsigned char)c);
                }
            }
            kprintf("\n");
        }
        
        kprintf("\n--- End of File ---\n");
        
        // Free the buffer
        free(file_buffer);
    } else {
        kprintf("\nUnknown command: %s\n", command);
    }
}

__attribute__((ms_abi)) [[noreturn]] void main(BOB* bob) {
    unsigned long long timeNow = RTC::getEpochTime();
    SetFramebuffer(bob->framebuffer);
    SetFont(bob->FontFile);
    Inferno(bob);
    unsigned long long render = timeNow-RTC::getEpochTime();
    // Once finished say hello and halt
    prInfo("kernel", "Done!");
    prInfo("kernel", "Boot time: %ds", render);
    // Buffer to store user input
    char inputBuffer[256];

    // Command prompt loop
    while (true) {
        kprintf("$ ");
        readSerial(inputBuffer, sizeof(inputBuffer));
        processCommand(inputBuffer);
    }

    // Fall back to halt loop if ACPI shutdown fails
    while (true) asm("hlt");
}

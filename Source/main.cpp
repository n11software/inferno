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
#include <Drivers/Storage/FS/EXT2.h>

// Forward declaration for our helper function
bool DirectCreateMapping(uint64_t cr3, uint64_t virtAddr, uint64_t physAddr);

// Forward declaration for SATA driver test function
extern "C" void test_sata_driver();

extern unsigned long long _InfernoEnd;
extern unsigned long long _InfernoStart;

extern "C" {
	void* malloc(size_t);
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
    } else if (strcmp(command, "ls") == 0) {
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
            // Try to detect the filesystem type
            int fs_type = ahci_detect_filesystem(port_num);
            
            // If it's an EXT2 filesystem (or we're assuming it for testing)
            if (fs_type == FS_TYPE_EXT2 || fs_type == FS_TYPE_EXT4) {
                // kprintf("Found EXT2/EXT4 filesystem (type: %d)\n", fs_type);
                // kprintf("Attempting to initialize EXT2 filesystem...\n");
                
                // Attempt to initialize the filesystem
                if (!FS::EXT2::Initialize(port_num)) {
                    kprintf("Standard initialization failed, testing different options...\n");
                    
                    // If standard initialization fails, let's try a direct sector read
                    ahci_device_t* device = ahci_get_device_info(port_num);
                    
                    // Ensure valid sector size
                    uint32_t sector_size = device->sector_size;
                    if (sector_size == 0) {
                        kprintf("Invalid sector size, using default 512 bytes\n");
                        sector_size = 512;
                    }
                    
                    // Try reading the first few sectors directly
                    void* buffer = malloc(sector_size * 4);  // Read 4 sectors
                    if (buffer) {
                        memset(buffer, 0, sector_size * 4);
                        kprintf("Reading first 4 sectors directly...\n");
                        
                        int result = ahci_read_sectors(port_num, 0, 4, buffer);
                        if (result == 0) {
                            kprintf("Successfully read first 4 sectors\n");
                            
                            // Dump the first 32 bytes for debugging
                            uint8_t* bytes = (uint8_t*)buffer;
                            kprintf("First 32 bytes of disk:\n");
                            for (int i = 0; i < 32; i++) {
                                kprintf("%02x ", bytes[i]);
                                if ((i + 1) % 16 == 0) kprintf("\n");
                            }
                            kprintf("\n");
                            
                            // Look for filesystem signatures
                            if (bytes[510] == 0x55 && bytes[511] == 0xAA) {
                                kprintf("Found boot sector signature (55 AA) at offset 510\n");
                            }
                        } else {
                            kprintf("Failed to read sectors, error: %d\n", result);
                        }
                        free(buffer);
                    }
                    
                    kprintf("Failed to initialize EXT2 filesystem\n");
                } else {
                    // kprintf("EXT2 filesystem initialized successfully\n");
                    
                    // Attempt to read and list the root directory
                    // if (!FS::EXT2::ReadRootDirectory(port_num)) {
                    //     kprintf("Failed to read root directory\n");
                    // }
                }
            } else {
                if (fs_type < 0) {
                    kprintf("Failed to detect filesystem (error: %d)\n", fs_type);
                } else {
                    kprintf("Filesystem type %d not supported by ls command\n", fs_type);
                    
                    // Print the detected filesystem type
                    switch (fs_type) {
                        case FS_TYPE_FAT:
                            kprintf("Detected FAT filesystem (generic)\n");
                            break;
                        case FS_TYPE_FAT12:
                            kprintf("Detected FAT12 filesystem\n");
                            break;
                        case FS_TYPE_FAT16:
                            kprintf("Detected FAT16 filesystem\n");
                            break;
                        case FS_TYPE_FAT32:
                            kprintf("Detected FAT32 filesystem\n");
                            break;
                        default:
                            kprintf("Unknown filesystem type: %d\n", fs_type);
                            break;
                    }
                }
            }
        } else {
            kprintf("No SATA devices found\n");
        }
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

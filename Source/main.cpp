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

// Forward declaration for our helper function
bool DirectCreateMapping(uint64_t cr3, uint64_t virtAddr, uint64_t physAddr);

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
		
		prInfo("paging", "Mapping framebuffer at 0x%x, size: 0x%x", fbAddr, fbSize);
		
		// Map each page of the framebuffer
		for (uint64_t offset = 0; offset < fbSize; offset += 0x1000) {
			Paging::MapPage(fbAddr + offset, fbAddr + offset);
		}
	}
	
	// Map a test page - but farther away from kernel and tables
	uint64_t testPhysAddr = 0x1000000; // 16MB mark, should be unused
	uint64_t testVirtAddr = 0x1400000; // 20MB mark, should be unmapped
	
	prInfo("paging", "Mapping test page: virtual 0x%x -> physical 0x%x", testVirtAddr, testPhysAddr);
	Paging::MapPage(testVirtAddr, testPhysAddr);
	
	// Enable paging with careful preparation 
	prInfo("kernel", "Preparing to enable paging, current code at %p", (void*)&&current_position);
current_position:
	// Make sure this code location is mapped
	Paging::MapPage((uint64_t)&&current_position & ~0xFFF, (uint64_t)&&current_position & ~0xFFF);
	
	prInfo("kernel", "Enabling paging...");
	Paging::Enable();
	
	if (Paging::IsEnabled()) {
		prInfo("kernel", "Paging successfully enabled");
		
		// Test that mapped page is accessible
		uint64_t* testPtr = (uint64_t*)testVirtAddr;
		*testPtr = 0xDEADBEEF; // Write test pattern
		
		if (*testPtr == 0xDEADBEEF) {
			prInfo("paging", "Virtual memory test passed: wrote and read 0xDEADBEEF at 0x%x", testVirtAddr);
		} else {
			prErr("paging", "Virtual memory test failed at 0x%x", testVirtAddr);
		}
		
		// Get physical address and verify
		uint64_t resolvedPhys = Paging::GetPhysicalAddress(testVirtAddr);
		prInfo("paging", "Virtual 0x%x resolves to physical 0x%x", testVirtAddr, resolvedPhys);
		
		// Clean up test page
		Paging::UnmapPage(testVirtAddr);
		prInfo("paging", "Unmapped test page at 0x%x", testVirtAddr);
	} else {
		prErr("kernel", "Failed to enable paging");
		while(1) asm("hlt");
	}
	
	// Initialize heap at 4MB with 1MB size initially
	Heap::Initialize(0x4000000, 0x100000);
	
	// Test heap allocator
	prInfo("kernel", "Testing heap allocator...");
	
	// Test 1: Basic allocation
	void* block1 = Heap::Allocate(1024);
	void* block2 = Heap::Allocate(2048);
	void* block3 = Heap::Allocate(4096);
	
	prInfo("heap", "Allocated blocks at: 0x%x, 0x%x, 0x%x", 
		   (uint64_t)block1, (uint64_t)block2, (uint64_t)block3);

	// Test 2: Free and reallocate
	Heap::Free(block2);  // Free middle block
	void* block4 = Heap::Allocate(1024);  // Should reuse part of freed space
	prInfo("heap", "Reallocated block at: 0x%x", (uint64_t)block4);

	// Test 3: Merge blocks
	Heap::Free(block1);
	Heap::Free(block4);  // Should merge with block1's space
	void* block5 = Heap::Allocate(2048);  // Should use merged space
	prInfo("heap", "Merged allocation at: 0x%x", (uint64_t)block5);

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
	} else {
		prErr("kernel", "ACPI initalization failed.");
	}

	PCI::init();

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
    if (strcmp(command, "shutdown") == 0) {
        kprintf("\nShutting down...\n");
        ACPI::shutdown();
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

    // Test brk
    uint64_t current_break;
    asm volatile(
        "push %%rdi\n"      // Save registers we'll use
        "push %%rax\n"
        "movq $0, %%rdi\n"  // Query current break
        "movq $12, %%rax\n" // SYS_BRK
        "int $0x80\n"       // Do syscall
        "movq %%rax, %0\n"  // Save result before restoring
        "pop %%rax\n"       // Restore registers
        "pop %%rdi"
        : "=m"(current_break) // Output to memory
        :: "memory"
    );
    
    prInfo("kernel", "Current break: 0x%x", current_break);

    // Set new break
    uint64_t new_break = current_break + 8192;
    asm volatile(
        "push %%rdi\n"      // Save registers we'll use
        "push %%rax\n"
        "movq %0, %%rdi\n"  // New break value
        "movq $12, %%rax\n" // SYS_BRK
        "int $0x80\n"       // Do syscall
        "pop %%rax\n"       // Restore registers
        "pop %%rdi"
        :: "m"(new_break) : "memory"
    );
    
    prInfo("kernel", "Set break to: 0x%x", new_break);

    // Test mmap
    uint64_t mmap_addr = 0;           // Let kernel choose address
    uint64_t mmap_size = 0x2000;      // 8KB
    uint64_t mmap_prot = 0x3;         // PROT_READ | PROT_WRITE
    uint64_t mmap_flags = 0x22;       // MAP_PRIVATE | MAP_ANONYMOUS
    uint64_t mmap_fd = -1;            // No file descriptor for anonymous mapping
    uint64_t mmap_offset = 0;         // No offset for anonymous mapping
    uint64_t mmap_result;

    asm volatile(
        "push %%rdi\n"      // Save registers we'll use
        "push %%rsi\n"
        "push %%rdx\n"
        "push %%r10\n"
        "push %%r8\n"
        "push %%r9\n"
        "push %%rax\n"
        "movq %1, %%rdi\n"  // addr
        "movq %2, %%rsi\n"  // len
        "movq %3, %%rdx\n"  // prot
        "movq %4, %%r10\n"  // flags
        "movq %5, %%r8\n"   // fd
        "movq %6, %%r9\n"   // offset
        "movq $9, %%rax\n"  // SYS_MMAP
        "int $0x80\n"       // Do syscall
        "movq %%rax, %0\n"  // Save result
        "pop %%rax\n"       // Restore registers
        "pop %%r9\n"
        "pop %%r8\n"
        "pop %%r10\n"
        "pop %%rdx\n"
        "pop %%rsi\n"
        "pop %%rdi"
        : "=m"(mmap_result)  // Output
        : "m"(mmap_addr), "m"(mmap_size), "m"(mmap_prot),
          "m"(mmap_flags), "m"(mmap_fd), "m"(mmap_offset)
        : "memory"
    );
    
    prInfo("kernel", "mmap returned: 0x%x", mmap_result);

    const char* str1 = "Hello, World!";
    const char* str2 = "Hello, World!";
    const char* str3 = "Hello, Inferno!";

    int result1 = memcmp(str1, str2, 13);
    int result2 = memcmp(str1, str3, 13);
    prInfo("kernel", "memcmp result1: %d", result1);
    prInfo("kernel", "memcmp result2: %d", result2);
    
    // Run our simplified VM aliasing test - this doesn't try to break down 2MB pages
    // or manipulate page tables directly
    prInfo("kernel", "Starting simplified VM aliasing test");
    if (SimpleVMAliasTest::Test()) {
        prInfo("kernel", "Virtual memory aliasing test PASSED");
    } else {
        prErr("kernel", "Virtual memory aliasing test FAILED");
    }

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

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
#include <Interrupts/APIC.hpp>
#include <Interrupts/DoublePageFault.hpp>
#include <Interrupts/Interrupts.hpp>
#include <Interrupts/PageFault.hpp>
#include <Interrupts/Syscall.hpp>
#include <Memory/Mem_.hpp>
#include <CPU/CPUID.h>
#include <Inferno/Log.h>
#include <Drivers/TTY/TTY.h>
#include <Memory/Memory.hpp>  // Add this include
#include <Memory/Paging.hpp>
#include <Memory/Heap.hpp>

// Drivers
#include <Drivers/ACPI/acpi.h>
#include <Drivers/RTC/RTC.h>
#include <Drivers/PCI/PCI.h>

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
    
    Memory::Initialize(nullptr, 2 * 1024 * 1024);
    Paging::Initialize();
    
	// 
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

    if (!Paging::IsEnabled()) {
        prErr("kernel", "Failed to enable paging");
        while(1) asm("hlt");
    }

	// Create IDT
	Interrupts::CreateIDT();
	// Load IDT
	Interrupts::LoadIDT();
	prInfo("idt", "initialized IDT");

	// Create a test ISR
	Interrupts::CreateISR(0x80, (void*)SyscallHandler);
	Interrupts::CreateISR(0x0E, (void*)PageFault);
	Interrupts::CreateISR(0x08, (void*)DoublePageFault);

	// Load APIC
	if (APIC::Capable()) {
			APIC::Enable();
			prInfo("apic", "initalized APIC");
	}

	PCI::init();

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

__attribute__((ms_abi)) [[noreturn]] void main(BOB* bob) {
	unsigned long long timeNow = RTC::getEpochTime();
	SetFramebuffer(bob->framebuffer);
	SetFont(bob->FontFile);
	Inferno(bob);
	unsigned long long render = timeNow-RTC::getEpochTime();
	// Once finished say hello and halt
	prInfo("kernel", "Done!");
	prInfo("kernel", "Boot time: %ds", render);

	// test malloc
	void* ptr = malloc(40000);
	prInfo("kernel", "malloc: 0x%x", (uint64_t)ptr);
	free(ptr);

	void *ptr2 = malloc(40000);
	prInfo("kernel", "malloc: 0x%x", (uint64_t)ptr2);
	free(ptr2);

	// Test syscall malloc with properly preserved registers
    uint64_t ptr_addr;
    asm volatile(
        "push %%rdi\n"        // Save registers we'll use
        "push %%rax\n"
        "movq $1024, %%rdi\n" // Size argument
        "movq $2, %%rax\n"    // SYS_MALLOC
        "int $0x80\n"         // Do syscall
        "movq %%rax, %0\n"    // Save result before restoring
        "pop %%rax\n"         // Restore registers
        "pop %%rdi"
        : "=m"(ptr_addr)      // Output to memory to avoid register constraints
        :: "memory"
    );
    
    prInfo("kernel", "syscall malloc returned: 0x%x", ptr_addr);

	const char* str1 = "Hello, World!";
	const char* str2 = "Hello, World!";
	const char* str3 = "Hello, Inferno!";

	int result1 = memcmp(str1, str2, 13);
	int result2 = memcmp(str1, str3, 13);
	prInfo("kernel", "memcmp result1: %d", result1);
	prInfo("kernel", "memcmp result2: %d", result2);

	while (true) asm("hlt");
}

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
#include <Memory/Memory.hpp>
#include <Memory/Paging.h>
#include <Memory/Heap.h>
#include <CPU/CPUID.h>
#include <Inferno/Log.h>
#include <Drivers/TTY/TTY.h>

// Drivers
#include <Drivers/RTC/RTC.h>
#include <Drivers/PCI/PCI.h>

extern unsigned long long _InfernoEnd;
extern unsigned long long _InfernoStart;

extern unsigned long long Free, Used;
PhysicalMemoryManager pmm;
Paging* paging;

__attribute__((sysv_abi)) void Inferno(BOB* bob) {
	prInfo("kernel", "Inferno kernel version 0.1alpha");
	kprintf("Copyright 2021-2024 N11 Software.\nCopyright 2018-2021 Ariston Lorenzo and Levi Hicks. All rights reserved.\nSee COPYRIGHT in the Inferno source tree for more information.\n");

	RTC::init();

	// CPU
	CPU::CPUDetect();


	// Create IDT
	Interrupts::CreateIDT();
	
	// Load IDT
	Interrupts::LoadIDT();
	prInfo("idt", "initialized IDT");

	// Create a test ISR
	Interrupts::CreateISR(0x80, (void*)SyscallHandler);
	Interrupts::CreateISR(0x0E, (void*)PageFault);
	Interrupts::CreateISR(0x08, (void*)DoublePageFault);


	uint64_t bitmap[128] = {0};
	
	kprintf("Init Memory\n");
	Memory::Init(bob);
	kprintf("Done\n");
	unsigned long long TM = Memory::GetSize();
	_InfernoStart = (unsigned long long)bob->KernelAddress;
	_InfernoEnd = (unsigned long long)bob->KernelSize;
    kprintf("InfernoStart: %x\n", &_InfernoStart);
    kprintf("InfernoEnd: %x\n", _InfernoEnd);

	pmm = PhysicalMemoryManager(bitmap, TM);
	pmm.lock_addresses(&_InfernoStart, (uint64_t)&_InfernoEnd-(uint64_t)&_InfernoStart*4096);
	paging = &Paging(pmm);
    paging->map_page((uint64_t)paging->pml4, (uint64_t)paging->pml4);
    paging->map_page((uint64_t)paging->pdpt, (uint64_t)paging->pdpt);
    paging->map_page((uint64_t)paging->pd, (uint64_t)paging->pd);
    paging->map_page((uint64_t)paging->pt, (uint64_t)paging->pt);
	uint64_t kernel_start = (uint64_t)bob->KernelAddress;
    uint64_t kernel_end = kernel_start + bob->KernelSize *4096;
    for (uint64_t addr = kernel_start; addr < kernel_end; addr += PAGE_SIZE) {
        paging->map_page(addr, addr);
    }

	// map fb
	{uint64_t fb_addr = (unsigned long long)bob->framebuffer->Address;
    uint64_t fb_size = bob->framebuffer->Width*bob->framebuffer->Height*4;
    for (uint64_t addr = fb_addr; addr < fb_addr + fb_size; addr += PAGE_SIZE) {
        paging->map_page(addr, addr);
    }
	pmm.lock_addresses((void*)fb_addr, fb_size/4096);}
	{uint64_t fb_addr = (unsigned long long)0x100000;
    uint64_t fb_size = bob->framebuffer->Width*bob->framebuffer->Height*4;
    for (uint64_t addr = fb_addr; addr < fb_addr + fb_size; addr += PAGE_SIZE) {
        paging->map_page(addr, addr);
    }
	pmm.lock_addresses((void*)fb_addr, fb_size/4096);}
	{uint64_t fb_addr = (unsigned long long)0x300000;
    uint64_t fb_size = bob->framebuffer->Width*bob->framebuffer->Height*4;
    for (uint64_t addr = fb_addr; addr < fb_addr + fb_size; addr += PAGE_SIZE) {
        paging->map_page(addr, addr);
    }
	pmm.lock_addresses((void*)fb_addr, fb_size/4096);}

	for (unsigned long long i=0;i<bob->MapSize/bob->DescriptorSize; i++) {
        MemoryDescriptor* descriptor = (MemoryDescriptor*)((unsigned long long)bob->MemoryMap + (i * bob->DescriptorSize));
        if (descriptor->Type != 7) {
			pmm.lock_addresses(descriptor->PhysicalStart, descriptor->NumberOfPages);
			// Map pages
			for (unsigned long long j = (uint64_t)descriptor->PhysicalStart; j < (uint64_t)descriptor->PhysicalStart + (uint64_t)descriptor->NumberOfPages * PAGE_SIZE; j += PAGE_SIZE) {
                paging->map_page(j, j);
            }
        }
    }
	kprintf("Enabling PageTable\n");
	paging->enable_paging();
	kprintf("Loading PageTable\n");
	paging->load_paging();
	InitializeHeap((void*)0xf000, 0x10);
    kprintf("Free: %m\n", Free);
    kprintf("Used: %m\n", Used);

	void* t = malloc(0x100);
	malloc(0x100);
	malloc(0x100);

	kprintf("TEST: %p\n", (unsigned long long) t);
	free(t);
	t = malloc(0x100);
	kprintf("Free: %p\n", (unsigned long long) t);

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


	unsigned long res = 0;
	asm volatile("int $0x80" : "=a"(res) : "a"(1), 
		"d"((unsigned long)"Hello from syscall\n\r"), 
		"D"((unsigned long)0) : "rcx", "r11", "memory");
	test();
}

__attribute__((ms_abi)) [[noreturn]] void main(BOB* bob) {
	SetFramebuffer(bob->framebuffer);
	SetFont(bob->FontFile);
	Inferno(bob);
	unsigned long long timeNow = RTC::getEpochTime();
	kprintf("Hello!\n");
	unsigned long long render = RTC::getEpochTime()-timeNow;
	prInfo("kernel", "Render time: %ds", render);

	// Once finished say hello and halt
	prInfo("kernel", "Done!");

	while (true) asm("hlt");
}

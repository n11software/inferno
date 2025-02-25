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

// Drivers
#include <Drivers/RTC/RTC.h>
#include <Drivers/PCI/PCI.h>

extern unsigned long long _InfernoEnd;
extern unsigned long long _InfernoStart;

__attribute__((sysv_abi)) void Inferno(BOB* bob) {
	// Initialize TTY
	setFont(bob->FontFile);
	setFramebuffer(bob->framebuffer);
	prInfo("kernel", "Inferno kernel version 0.1alpha");
	kprintf("Copyright 2021-2025 N11 Software.\nCopyright 2018-2021 Ariston Lorenzo and Levi Hicks. All rights reserved.\nSee COPYRIGHT in the Inferno source tree for more information.\n");

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

	while (1) {
		unsigned long res = 0;
		asm volatile("int $0x80" : "=a"(res) : "a"(1), 
			"d"((unsigned long)"Hello from syscall\n\r"), 
			"D"((unsigned long)0) : "rcx", "r11", "memory");
	}

	while (true) asm("hlt");
}

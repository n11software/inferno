#pragma once

#include <Drivers/ACPI/acpi.h>

namespace APIC {
	bool Capable();
	void SetBase(unsigned int base);
	unsigned int GetBase();
	void Write(unsigned int reg, unsigned int value);
	unsigned int Read(unsigned int reg);
	void Enable();
	
	// PIC Management
	void DisablePIC();
	
	// APIC Initialization
	bool Initialize();
	void SetupLocalAPIC();
	void ConfigureSpuriousInterrupts();
	
	// Timer functions
	void ConfigureTimer(uint32_t divisor);
	void SetTimerCount(uint32_t count);
	
	// IRQ routing
	void MapIRQ(uint8_t irq, uint8_t vector);
	
	// Status checks
	bool IsEnabled();
	uint32_t GetVersion();
}
//========= Copyright N11 Software, All rights reserved. ============//
//
// File: Interrupts.cpp
// Purpose:
// Maintainer: FiReLScar
//
//===================================================================//

#include <Interrupts/Interrupts.hpp>

namespace Interrupts {
	Entry ISR[256];
	Table IDT;

	void CreateIDT() {
		IDT = {
			(unsigned short)(sizeof(ISR) - 1),
			(unsigned long)&ISR[0]
		};
	}

	void Enable() {
		asm("sti");
	}

	void Disable() {
		asm("cli");
	}

	void LoadIDT() {
		__asm__ volatile("lidt %0" :: "m"(IDT));
		Enable();
	}

	void CreateISR(unsigned char index, void* handler) {
		Entry* entry = &ISR[index];
		entry->isrLow = (unsigned long long)handler & 0xFFFF;
		entry->cs = 0x8;
		entry->ist = 0;
		entry->attributes = 0x8E;
		entry->isrMid = ((unsigned long long)handler >> 16) & 0xFFFF;
		entry->isrHigh = ((unsigned long long)handler >> 32) & 0xFFFFFFFF;
		entry->null = 0;
	}
}

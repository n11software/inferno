#include <Interrupts/APIC.hpp>
#include <CPU/CPUID.h>

namespace APIC {
	bool Capable() {
		unsigned long eax, unused, edx;
		cpuid(1, eax, unused, unused, edx);
		return edx & 1 << 9;
	}

	void SetBase(unsigned int base) {
		unsigned int edx = 0;
		unsigned int eax = (base & 0xfffff0000) | 0x800;
		#ifdef __PHYSICAL_MEMORY_EXTENSION__
			edx = (base >> 32) & 0x0f;
		#endif
		asm volatile("wrmsr" :: "a"(0x1B), "d"(eax), "c"(edx));
	}

	unsigned int GetBase() {
		unsigned int eax, edx;
		asm volatile("wrmsr" :: "a"(0x1B), "d"(eax), "c"(edx));
		#ifdef __PHYSICAL_MEMORY_EXTENSION__
			return (eax & 0xfffff000) | ((edx & 0x0f) << 32);
		#else
			return (eax & 0xfffff000);
		#endif
	}

	void Write(unsigned int reg, unsigned int value) {
		unsigned int volatile* apic = (unsigned int volatile*)GetBase();
		apic[0] = (reg & 0xff) << 4;
		apic[4] = value;
	}

	unsigned int Read(unsigned int reg) {
		unsigned int volatile* apic = (unsigned int volatile*)GetBase();
		apic[0] = (reg & 0xff) << 4;
		return apic[4];
	}

	void Enable() {
		SetBase(GetBase());
		Write(0xF0, Read(0xF0) | 0x100);
	}
}

#pragma once

namespace APIC {
	bool Capable();
	void SetBase(unsigned int base);
	unsigned int GetBase();
	void Write(unsigned int reg, unsigned int value);
	unsigned int Read(unsigned int reg);
	void Enable();
}
//========= Copyright N11 Software, All rights reserved. ============//
//
// File: access.cpp
// Purpose: PCI configuration access functions
// Maintainer: aristonl
//
//===================================================================//

#include <Drivers/PCI/PCI.h>
#include <Inferno/Log.h>
#include <Inferno/IO.h>
#include <Inferno/stdint.h>

namespace PCI {
uint16_t read_word(uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset) {
	uint64_t address;
	uint64_t lbus = (uint64_t)bus;
	uint64_t lslot = (uint64_t)slot;
	uint64_t lfunc = (uint64_t)func;
	uint16_t tmp = 0;

	address = (uint64_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) |
						 (offset & 0xfc) | ((uint32_t)0x80000000));
	outl(0xCF8, address);
	tmp = (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xffff);

	return tmp;
}
}

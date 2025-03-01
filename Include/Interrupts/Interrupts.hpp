//========= Copyright N11 Software, All rights reserved. ============//
//
// File: Interrupts.hpp
// Purpose:
// Maintainer: FiReLScar
//
//===================================================================//

#pragma once

namespace Interrupts {
	typedef struct {
		unsigned short isrLow, cs;
		unsigned char ist, attributes;
		unsigned short isrMid;
		unsigned int isrHigh, null;
	} __attribute__((packed)) Entry;

	typedef struct {
		unsigned short limit;
		unsigned long long base;
	} __attribute__((packed)) Table;

	void CreateIDT(), LoadIDT();
	void CreateISR(unsigned char index, void* handler);
	void Enable(), Disable();
}
//========= Copyright N11 Software, All rights reserved. ============//
//
// File: CPUID.cpp
// Purpose:
// Maintainer: atl
//
//===================================================================//

#include <Inferno/Log.h>
#include <CPU/CPUID.h>
#include <CPU/VendorID.h>
#include <Memory/Mem_.hpp>

namespace CPU {
	static char CPUModel[48];

	/* returns the CPU vendor ID */
	const char *VendorID() {
		unsigned long ebx, unused;
		cpuid(0, unused, ebx, unused, unused);	
		switch (ebx) {
			case 0x756e6547: /* Intel Magic Code */
				return CPUID_VENDOR_INTEL;
			case 0x68747541: /* AMD Magic Code */
				return CPUID_VENDOR_AMD;
			default:
				return "Unknown";
		}
	}

	char *Model() {
		int b, c, d, e;
		int family, model, stepping;
        	unsigned int id;
		unsigned int brand[12];

		cpuid(0x80000002, brand[0], brand[1], brand[2], brand[3]);
		cpuid(0x80000003, brand[4], brand[5], brand[6], brand[7]);
		cpuid(0x80000004, brand[8], brand[9], brand[10], brand[11]);
	
		cpuid(1, b, c, d, e);
	
		id = b;

		stepping = b & 0xF;
		model = (b >> 4) & 0xF;
		family = (b >> 8) & 0xF;	        

		prInfo("cpu", "CPU0: Origin=%s  Id=0x%x  Family=0x%x  Model=0x%x  Stepping=%d", VendorID(), id, family, model, stepping);
		prInfo("cpu", "CPU0: %s", (char *)brand);

		return (char *)brand;
	}
}

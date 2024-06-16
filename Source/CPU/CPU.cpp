//========= Copyright N11 Software, All rights reserved. ============//
//
// File: CPU.cpp
// Maintainer: atl
//
//===================================================================//

#include <Inferno/Log.h>
#include <CPU/CPU.h>
#include <CPU/CPUID.h>
#include <CPU/VendorID.h>

namespace CPU {
	void CPUDetect() {
		const char *vendor = VendorID();
		if (vendor == CPUID_VENDOR_INTEL) {
			IntelHandler();
		} else if (vendor == CPUID_VENDOR_AMD || vendor == CPUID_VENDOR_AMD_ALT) {
			AMDHandler();
		} else {
			UnknownHandler();
		}
	}

	void IntelHandler() {
		/* determine the model */
		char *model = Model();
	}

	void AMDHandler() {
		char *model = Model();
	}

	void UnknownHandler() {
		prErr("cpu", "CPU0: Unknown CPU detected");
		prWarn("cpu", "CPU0: attempting to get cpu features anyways");
	}
}

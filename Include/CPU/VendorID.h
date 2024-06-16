//========= Copyright N11 Software, All rights reserved. ============//
//
// File: VendorID.h
// Purpose: CPU Vendor ID defines
// Maintainer: atl
//
//===================================================================//

#ifndef VENDORID_H
#define VENDORID_H

#define CPUID_VENDOR_INTEL "GenuineIntel"
#define CPUID_VENDOR_AMD "AuthenticAMD"
#define CPUID_VENDOR_AMD_ALT "AMDisbetter!" /* only on some AMD CPUs */

/* vendor id's from virtual machines */
#define CPUID_VENDOR_QEMU "TCGTCGTCGTCG"
#define CPUID_VENDOR_KVM " KVMKVMKVM  "
#define CPUID_VENDOR_VMWARE "VMwareVMware"
#define CPUID_VENDOR_VBOX "VBoxVBoxVBox"

#endif //VENDORID_H

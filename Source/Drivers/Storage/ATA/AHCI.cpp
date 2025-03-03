#include <Drivers/Storage/AHCI/AHCI.h>
#include <Drivers/PCI/PCI.h>

#include <Inferno/Log.h>

#include <Inferno/stdint.h>

// IMPORTANT: This is just a stub for backward compatibility.
// The real implementation is in kernel/Source/Drivers/Storage/AHCI/AHCI.cpp
// This stub should not be used directly; instead include <Drivers/Storage/AHCI/AHCI.h>
// and use the functions declared there.

// Forward declare the implementation from Storage/AHCI/AHCI.cpp
// We don't need to implement it here since it's already defined there
extern int ahci_init(uint16_t vendor_id, uint16_t device_id, uint16_t bus = 0, uint16_t device = 0, uint16_t function = 0);

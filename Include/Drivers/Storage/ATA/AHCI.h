#pragma once

#include <Inferno/stdint.h>
#include <Drivers/Storage/AHCI/AHCI.h>

// This header now just includes the main AHCI header for compatibility
// All AHCI functionality is defined in <Drivers/Storage/AHCI/AHCI.h>

// Keep the original function for backward compatibility
int ahci_init(uint16_t vendor_id, uint16_t device_id);

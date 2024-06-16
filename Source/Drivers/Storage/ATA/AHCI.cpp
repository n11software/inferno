#include <Drivers/Storage/AHCI/AHCI.h>
#include <Drivers/PCI/PCI.h>

#include <Inferno/Log.h>

#include <Inferno/stdint.h>

int ahci_init(uint16_t vendor_id, uint16_t device_id) {
	if (vendor_id == 0x8086 && device_id == 0x2922) {
		prInfo("ahci", "Initializing AHCI device, Intel ICH9 6 Port SATA AHCI Controller.");
	}

	return 0;
}

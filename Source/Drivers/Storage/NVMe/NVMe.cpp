#include <Drivers/Storage/NVMe/NVMe.h>
#include <Drivers/PCI/PCI.h>
#include <Inferno/Log.h>

#include <Inferno/stdint.h>

int nvme_init(uint16_t vendor_id, uint16_t device_id) {
	if (vendor_id == 0x1b36 && device_id == 0x0010) {
		prInfo("nvme", "Initializing NVMe device, QEMU NVM Express Controller");
	} else if (vendor_id == 0x1c5c && device_id == 0x1627) {
		prInfo("nvme", "Initializing NVMe device, SK hynix PC601 NVMe Solid State Drive");
	} else {
		prWarn("nvme", "NVMe device found, no specific driver, using generic.");
		prDebug("nvme", "NVMe device - vendor ID: %x - device ID: %x", vendor_id, device_id);
	}
	return 0;
}

//========= Copyright N11 Software, All rights reserved. ============//
//
// File: pci.h
// Purpose: Header file for anything PCI-related
// Maintainer: atl
//
//===================================================================//

#pragma once

#include <Inferno/stdint.h>

#include <Drivers/Storage/NVMe/NVMe.h>

namespace PCI {
	/*
	 * We need this for UEFI to detect if there is even a PCI bridge at all.
	 * For BIOS all we need to do is ping the BIOS or read from ACPI. Both of
	 * which isn't as reliable on a UEFI machine. Unfortunately we can't determine
	 * what exact Config Mech to use in UEFI.
	 *
	 * See https://uefi.org/specs/UEFI/2.10/14_Protocols_PCI_Bus_Support.html
	 */
	// EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL

	uint16_t read_word(uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset);

	uint16_t get_vendor_id(uint16_t bus, uint16_t dev, uint16_t func);
	void check_device(uint16_t bus, uint16_t dev);
	void check_bus(uint16_t bus);

	void init();

	typedef struct {
		uint16_t vendor_id;
		uint16_t device_id;
		uint16_t class_id;
		uint16_t subclass_id;
	} pci_device_t;

	struct pci_driver {
		uint16_t class_id;
		uint16_t subclass_id;
		void (*init)(pci_device_t *);
	};

	#define PCI_NUM_DRIVERS (sizeof(drivers) / sizeof(struct pci_driver *))

	void load_pci_drivers(PCI::pci_device_t* pci_device);
}

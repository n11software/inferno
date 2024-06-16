//========= Copyright N11 Software, All rights reserved. ============//
//
// File: acpi.h
// Purpose: Main header file for the ACPI driver
// Maintainer: atl
//
//===================================================================//

#include <Inferno/stdint.h>

/* ACPI v1.0 */
struct rsdp_descriptor {
	char signature[8];
	uint8_t checksum;
	char oem_id[6];
	uint8_t revision;
	uint32_t rsdt_addr;
} __attribute__ ((packed));

/* ACPI v2.0 */
struct rsdp_descriptor_2 {
	rsdp_descriptor first_part;

	uint32_t length;
	uint64_t xsdt_addr;
	uint8_t ext_checksum;
	uint8_t reserved[3];
} __attribute__ ((packed));

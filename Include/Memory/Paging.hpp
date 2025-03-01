#pragma once
#include <Inferno/stdint.h>

namespace Paging {
	struct PageTable {
		uint64_t entries[512];
	} __attribute__((packed));

	void Initialize();
	void Enable();
	bool IsEnabled();
	void MapPage(uint64_t virtual_addr, uint64_t physical_addr);
	void UnmapPage(uint64_t virtual_addr);
	uint64_t GetPhysicalAddress(uint64_t virtual_addr);
}

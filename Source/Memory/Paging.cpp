#include <Memory/Paging.hpp>
#include <Memory/Memory.hpp>
#include <Memory/Mem_.hpp>
#include <Inferno/Log.h>

namespace Paging {
	#define PAGING_TABLES_BASE 0x300000

	static PageTable* const pml4 = (PageTable*)PAGING_TABLES_BASE;
	static PageTable* const pdp  = (PageTable*)(PAGING_TABLES_BASE + 0x1000);
	static PageTable* const pd   = (PageTable*)(PAGING_TABLES_BASE + 0x2000);
	static PageTable* const pt   = (PageTable*)(PAGING_TABLES_BASE + 0x3000);

	void Initialize() {
		// Get existing page tables
		uint64_t current_cr3;
		asm volatile("mov %%cr3, %0" : "=r"(current_cr3));
		prInfo("paging", "Current tables at 0x%x", current_cr3);

		// Zero our new tables
		memset(pml4, 0, 4096);
		memset(pdp, 0, 4096);
		memset(pd, 0, 4096);
		memset(pt, 0, 4096);

		// Identity map first 2MB only
		pml4->entries[0] = (uint64_t)pdp | 0x3;
		pdp->entries[0] = (uint64_t)pd | 0x3;
		pd->entries[0] = (uint64_t)pt | 0x3;

		// Map first 512 pages (2MB)
		for(int i = 0; i < 512; i++) {
			pt->entries[i] = (i * 0x1000) | 0x3;
		}

		// Keep using existing tables for now
		prInfo("paging", "Page tables initialized");
	}

	bool IsEnabled() {
		uint64_t cr0;
		asm volatile("mov %%cr0, %0" : "=r"(cr0));
		return (cr0 & (1ULL << 31)) != 0;
	}

	void MapPage(uint64_t virtual_addr, uint64_t physical_addr) {
		// Implementation for later
	}
}

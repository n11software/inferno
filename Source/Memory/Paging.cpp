#include <Memory/Paging.hpp>
#include <Memory/Memory.hpp>
#include <Memory/Mem_.hpp>
#include <Inferno/Log.h>

// These globals must be accessible from any namespace
extern "C" {
    extern unsigned long long _InfernoStart;
    extern unsigned long long _InfernoEnd;
}

namespace Paging {
	#define PAGING_TABLES_BASE 0x300000
	#define PAGE_PRESENT 0x1
	#define PAGE_WRITABLE 0x2
	#define PAGE_USER 0x4
	#define PAGE_SIZE_BIT 0x80
	#define PAGE_DEFAULT (PAGE_PRESENT | PAGE_WRITABLE)

	static PageTable* const pml4 = (PageTable*)PAGING_TABLES_BASE;
	static PageTable* const pdp  = (PageTable*)(PAGING_TABLES_BASE + 0x1000);
	static PageTable* const pd   = (PageTable*)(PAGING_TABLES_BASE + 0x2000);
	static PageTable* const pt   = (PageTable*)(PAGING_TABLES_BASE + 0x3000);

	// Extract page table indices from virtual address
	static inline uint16_t GetPML4Index(uint64_t addr) { return (addr >> 39) & 0x1FF; }
	static inline uint16_t GetPDPIndex(uint64_t addr)  { return (addr >> 30) & 0x1FF; }
	static inline uint16_t GetPDIndex(uint64_t addr)   { return (addr >> 21) & 0x1FF; }
	static inline uint16_t GetPTIndex(uint64_t addr)   { return (addr >> 12) & 0x1FF; }

	// Special function that must be in identity-mapped memory
	__attribute__((section(".identtext"))) static void SafeCR3Load(uint64_t cr3) {
		asm volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
	}

	void Initialize() {
		// Important: In UEFI x64, paging is already enabled
		// We should examine existing tables first
		uint64_t current_cr3;
		asm volatile("mov %%cr3, %0" : "=r"(current_cr3));
		prInfo("paging", "Current tables at 0x%x", current_cr3);

		 // Disable interrupts during paging setup - CRITICAL for stability
		asm volatile("cli");

		// Zero our new tables
		memset(pml4, 0, 4096);
		memset(pdp, 0, 4096);
		memset(pd, 0, 4096);
		memset(pt, 0, 4096);

		// Instead of recreating all mappings from scratch, we'll copy relevant entries 
		// from the existing page tables to preserve UEFI memory mapping

		// First copy the existing PML4 entries
		PageTable* current_pml4 = (PageTable*)(current_cr3 & ~0xFFF);
		
		// For safety, preserve higher-half mapping to ensure boot services remain accessible
		// Copy entries 256-511 (higher half)
		for (int i = 256; i < 512; i++) {
			if (current_pml4->entries[i] & PAGE_PRESENT) {
				pml4->entries[i] = current_pml4->entries[i];
			}
		}

		// Now set up our basic identity mapping for the lower 16MB to ensure we cover all kernel areas
		pml4->entries[0] = (uint64_t)pdp | PAGE_DEFAULT;
		pdp->entries[0] = (uint64_t)pd | PAGE_DEFAULT;
		
		// Map first 8 page directories (16MB) to be safe
		for (int i = 0; i < 8; i++) {
			PageTable* current_pt;
			
			// Use our predefined PT for first 2MB, allocate others
			if (i == 0) {
				current_pt = pt;
			} else {
				current_pt = (PageTable*)Memory::RequestPage();
				if (!current_pt) {
					prErr("paging", "Failed to allocate PT for region %d", i);
					asm volatile("sti"); // Re-enable interrupts before returning
					return;
				}
			}
			
			memset(current_pt, 0, 4096);
			pd->entries[i] = (uint64_t)current_pt | PAGE_DEFAULT;
			
			// Map all pages in this table (2MB per table)
			for (int j = 0; j < 512; j++) {
				uint64_t phys_addr = (i * 512 + j) * 0x1000;
				current_pt->entries[j] = phys_addr | PAGE_DEFAULT;
			}
			
			prInfo("paging", "Mapped 2MB region at 0x%x", i * 0x200000);
		}

		// Get proper kernel bounds
		uint64_t kernel_start = 0x100000;  // Known physical start from UEFI loader
		uint64_t kernel_end = kernel_start + ((uint64_t)&_InfernoEnd - (uint64_t)&_InfernoStart);
		kernel_end = (kernel_end + 0x1000) & ~0xFFF; // Align to page boundary

		prInfo("paging", "Identity mapping kernel 0x%x - 0x%x", kernel_start, kernel_end);
		
		// Make sure kernel is properly mapped
		for (uint64_t addr = kernel_start; addr < kernel_end; addr += 0x1000) {
			MapPage(addr, addr);
		}

		// Map the page tables themselves
		for (uint64_t addr = PAGING_TABLES_BASE; addr < PAGING_TABLES_BASE + 0x10000; addr += 0x1000) {
			MapPage(addr, addr);
		}

		// Map the current function to ensure it stays accessible after CR3 reload
		uint64_t code_page = (uint64_t)&Initialize & ~0xFFF;
		MapPage(code_page, code_page);
		
		// Map the stack area (critical for function returns!)
		uint64_t rsp;
		asm volatile("mov %%rsp, %0" : "=r"(rsp));
		uint64_t stack_page = rsp & ~0xFFF;
		
		// Map several pages around the stack pointer to be safe
		for (int i = -8; i <= 8; i++) {
			uint64_t page = stack_page + (i * 0x1000);
			MapPage(page, page);
		}
		prInfo("paging", "Mapped current stack region around 0x%x", stack_page);

		// Map the framebuffer region (assuming it's somewhere above 16MB)
		// This is crucial for continued console output
		for (uint64_t addr = 0xFD000000; addr < 0xFE000000; addr += 0x1000) {
			PageTable* src_pml4 = (PageTable*)(current_cr3 & ~0xFFF);
			uint16_t pml4_idx = GetPML4Index(addr);
			
			if (src_pml4->entries[pml4_idx] & PAGE_PRESENT) {
				PageTable* src_pdp = (PageTable*)(src_pml4->entries[pml4_idx] & ~0xFFF);
				uint16_t pdp_idx = GetPDPIndex(addr);
				
				if (src_pdp->entries[pdp_idx] & PAGE_PRESENT) {
					pml4->entries[pml4_idx] = src_pml4->entries[pml4_idx];
				}
			}
		}

		// Map SafeCR3Load function ensuring it's accessible during CR3 change
		uint64_t trampoline_page = (uint64_t)&SafeCR3Load & ~0xFFF;
		MapPage(trampoline_page, trampoline_page);
		
		prInfo("paging", "Page tables initialized");
		
		// Re-enable interrupts before returning
		asm volatile("sti");
	}

	bool IsEnabled() {
		uint64_t cr0;
		asm volatile("mov %%cr0, %0" : "=r"(cr0));
		return (cr0 & (1ULL << 31)) != 0;
	}

	void MapPage(uint64_t virtual_addr, uint64_t physical_addr) {
		// Align addresses to page boundaries
		virtual_addr &= ~0xFFF;
		physical_addr &= ~0xFFF;
		
		#ifdef DEBUG_PAGING
		prInfo("paging", "MapPage: mapping 0x%x to 0x%x", virtual_addr, physical_addr);
		#endif
		
		uint16_t pml4_idx = GetPML4Index(virtual_addr);
		uint16_t pdp_idx = GetPDPIndex(virtual_addr);
		uint16_t pd_idx = GetPDIndex(virtual_addr);
		uint16_t pt_idx = GetPTIndex(virtual_addr);
		
		// Check if PML4 entry exists
		if (!(pml4->entries[pml4_idx] & PAGE_PRESENT)) {
			PageTable* new_pdp = (PageTable*)Memory::RequestPage();
			if (!new_pdp) {
				prErr("paging", "Failed to allocate PDP table");
				return;
			}
			memset(new_pdp, 0, 4096);
			pml4->entries[pml4_idx] = (uint64_t)new_pdp | PAGE_DEFAULT;
		}
		
		// Get PDP table and check if entry exists
		PageTable* pdp_table = (PageTable*)(pml4->entries[pml4_idx] & ~0xFFF);
		if (!(pdp_table->entries[pdp_idx] & PAGE_PRESENT)) {
			PageTable* new_pd = (PageTable*)Memory::RequestPage();
			if (!new_pd) {
				prErr("paging", "Failed to allocate PD table");
				return;
			}
			memset(new_pd, 0, 4096);
			pdp_table->entries[pdp_idx] = (uint64_t)new_pd | PAGE_DEFAULT;
		}
		
		// Get PD table and check if entry exists
		PageTable* pd_table = (PageTable*)(pdp_table->entries[pdp_idx] & ~0xFFF);
		
		// Check for 2MB page
		if ((pd_table->entries[pd_idx] & PAGE_PRESENT) && 
			(pd_table->entries[pd_idx] & PAGE_SIZE_BIT)) {
			// This is a 2MB page - we'll avoid touching it 
			#ifdef DEBUG_PAGING
			prInfo("paging", "Skipping attempt to modify 2MB page at virtual addr 0x%x", 
				   virtual_addr & ~0x1FFFFF);
			#endif
			return;
		}
		
		// Check if PD entry exists
		if (!(pd_table->entries[pd_idx] & PAGE_PRESENT)) {
			PageTable* new_pt = (PageTable*)Memory::RequestPage();
			if (!new_pt) {
				prErr("paging", "Failed to allocate PT table");
				return;
			}
			memset(new_pt, 0, 4096);
			pd_table->entries[pd_idx] = (uint64_t)new_pt | PAGE_DEFAULT;
		}
		
		// Get PT table and set the entry
		PageTable* pt_table = (PageTable*)(pd_table->entries[pd_idx] & ~0xFFF);
		pt_table->entries[pt_idx] = physical_addr | PAGE_DEFAULT;
		
		// Flush TLB for this specific address
		asm volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
		
		#ifdef DEBUG_PAGING
		// Verify our mapping worked
		uint64_t check_addr = GetPhysicalAddress(virtual_addr);
		if (check_addr != physical_addr) {
			prErr("paging", "MapPage FAILED: 0x%x -> 0x%x (expected 0x%x)", 
				   virtual_addr, check_addr, physical_addr);
		}
		#endif
	}

	void UnmapPage(uint64_t virtual_addr) {
		virtual_addr &= ~0xFFF;
		
		uint16_t pml4_idx = GetPML4Index(virtual_addr);
		uint16_t pdp_idx = GetPDPIndex(virtual_addr);
		uint16_t pd_idx = GetPDIndex(virtual_addr);
		uint16_t pt_idx = GetPTIndex(virtual_addr);
		
		// Check if PML4 entry exists
		if (!(pml4->entries[pml4_idx] & PAGE_PRESENT)) {
			return; // Nothing to unmap
		}
		
		// Get PDP table
		PageTable* pdp_table = (PageTable*)(pml4->entries[pml4_idx] & ~0xFFF);
		if (!(pdp_table->entries[pdp_idx] & PAGE_PRESENT)) {
			return; // Nothing to unmap
		}
		
		// Get PD table
		PageTable* pd_table = (PageTable*)(pdp_table->entries[pdp_idx] & ~0xFFF);
		if (!(pd_table->entries[pd_idx] & PAGE_PRESENT)) {
			return; // Nothing to unmap
		}
		
		// Get PT table and clear the entry
		PageTable* pt_table = (PageTable*)(pd_table->entries[pd_idx] & ~0xFFF);
		pt_table->entries[pt_idx] = 0;
		
		// Flush TLB for this address
		asm volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
	}

	void Enable() {
		uint64_t pml4_addr = (uint64_t)pml4;
		prInfo("paging", "Loading page tables from 0x%x", pml4_addr);
		
		// Critical: disable interrupts during CR3 switch
		asm volatile("cli");
		
		// Call our safe CR3 loading function which is identity-mapped
		SafeCR3Load(pml4_addr);
		
		// If we get here, the CR3 switch was successful
		prInfo("paging", "Successfully loaded new page tables");
		
		// Re-enable interrupts
		asm volatile("sti");
		
		// Verify paging is enabled (it should be in x64 mode)
		if (IsEnabled()) {
			prInfo("paging", "Paging verified as enabled");
		} else {
			prErr("paging", "WARNING: Paging appears to be disabled - this should not happen in x64 mode");
		}
	}
	
	uint64_t GetPhysicalAddress(uint64_t virtual_addr) {
		 // Get current CR3 value
		uint64_t current_cr3;
		asm volatile("mov %%cr3, %0" : "=r"(current_cr3));
		
		// Use the current CR3 value to find the actual page tables in use
		PageTable* actual_pml4 = (PageTable*)(current_cr3 & ~0xFFF);
		
		uint16_t pml4_idx = GetPML4Index(virtual_addr);
		uint16_t pdp_idx = GetPDPIndex(virtual_addr);
		uint16_t pd_idx = GetPDIndex(virtual_addr);
		uint16_t pt_idx = GetPTIndex(virtual_addr);
		uint64_t offset = virtual_addr & 0xFFF;
		
		// Walk the page tables
		if (!(actual_pml4->entries[pml4_idx] & PAGE_PRESENT)) {
			return 0;
		}
		
		PageTable* pdp_table = (PageTable*)(actual_pml4->entries[pml4_idx] & ~0xFFF);
		
		if (!(pdp_table->entries[pdp_idx] & PAGE_PRESENT)) {
			return 0;
		}
		
		PageTable* pd_table = (PageTable*)(pdp_table->entries[pdp_idx] & ~0xFFF);
		
		if (!(pd_table->entries[pd_idx] & PAGE_PRESENT)) {
			return 0;
		}
		
		// Check if this is a 2MB page
		if (pd_table->entries[pd_idx] & PAGE_SIZE_BIT) {
			uint64_t phys = (pd_table->entries[pd_idx] & ~0x1FFFFF) + (virtual_addr & 0x1FFFFF);
			return phys;
		}
		
		PageTable* pt_table = (PageTable*)(pd_table->entries[pd_idx] & ~0xFFF);
		
		if (!(pt_table->entries[pt_idx] & PAGE_PRESENT)) {
			return 0;
		}
		
		uint64_t phys_addr = (pt_table->entries[pt_idx] & ~0xFFF) + offset;
		return phys_addr;
	}
}

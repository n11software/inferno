#include <Memory/Paging.h>
#include <Memory/Memory.hpp>
#include <Memory/Mem_.hpp>
#include <stdint.h>
#include <stddef.h>
#include <Drivers/TTY/COM.h>

PhysicalMemoryManager::PhysicalMemoryManager(uint64_t* bitmap, uint64_t total_memory) {
    memory_bitmap = bitmap;
    total_pages = total_memory / PAGE_SIZE;
    kprintf("PhysicalMemoryManager initialized with %d total pages\n", total_pages);
}

extern "C" unsigned long long Free, Used;

void* PhysicalMemoryManager::alloc_page() {
    for (uint64_t i = 0; i < total_pages; i++) {
        if (!(memory_bitmap[i / 64] & (1ULL << (i % 64)))) {
            memory_bitmap[i / 64] |= (1ULL << (i % 64));
			Free -= 4096;
			Used += 4096;
            return (void*)(i * PAGE_SIZE);
        }
    }
    return nullptr; // Out of memory
}

void PhysicalMemoryManager::free_page(void* page) {
    uint64_t index = (uint64_t)page / PAGE_SIZE;
    memory_bitmap[index / 64] &= ~(1ULL << (index % 64));
	Free += 4096;
	Used -= 4096;
}

void PhysicalMemoryManager::lock_address(void* address) {
    uint64_t index = (uint64_t)address / PAGE_SIZE;
    memory_bitmap[index / 64] |= (1ULL << (index % 64));
    // kprintf("Locked address: %p (page index %d)\n", address, index);
	Free -= 4096;
	Used += 4096;
}

void PhysicalMemoryManager::lock_addresses(void* address, uint64_t size) {
    uint64_t start_page = (uint64_t)address / PAGE_SIZE;
    uint64_t end_page = ((uint64_t)address + size + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint64_t i = start_page; i < end_page; i++) {
        memory_bitmap[i / 64] |= (1ULL << (i % 64));
        // kprintf("Locked page index: %d\n", i);
    }
}

// Implementation of Paging methods

Paging::Paging(PhysicalMemoryManager& pmm) : pmm(pmm) {
    pml4 = (uint64_t*)pmm.alloc_page();
    pdpt = (uint64_t*)pmm.alloc_page();
    pd = (uint64_t*)pmm.alloc_page();
    pt = (uint64_t*)pmm.alloc_page();

    if (!pml4 || !pdpt || !pd || !pt) {
        kprintf("Error: Failed to allocate page tables\n");
        return;
    }

    kprintf("Page tables allocated\n");
    kprintf("PML4: %p, PDPT: %p, PD: %p, PT: %p\n", pml4, pdpt, pd, pt);

    memset(pml4, 0, PAGE_SIZE);
    memset(pdpt, 0, PAGE_SIZE);
    memset(pd, 0, PAGE_SIZE);
    memset(pt, 0, PAGE_SIZE);

    for (int i = 0; i < 512; i++) {
        pt[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITE;
    }

    pd[0] = (uint64_t)pt | PAGE_PRESENT | PAGE_WRITE;
    pdpt[0] = (uint64_t)pd | PAGE_PRESENT | PAGE_WRITE;
    pml4[0] = (uint64_t)pdpt | PAGE_PRESENT | PAGE_WRITE;

    kprintf("Paging structures set up\n");
}

void Paging::map_page(uint64_t phys_addr, uint64_t virt_addr) {
    uint64_t pml4_index = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_index = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_index = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_index = (virt_addr >> 12) & 0x1FF;

    uint64_t* pdpt_entry = &pml4[pml4_index];
    if (!(*pdpt_entry & PAGE_PRESENT)) {
        *pdpt_entry = (uint64_t)pmm.alloc_page() | PAGE_PRESENT | PAGE_WRITE;
        memset((void*)(*pdpt_entry & ~0xFFF), 0, PAGE_SIZE);
    }

    uint64_t* pd_entry = &((uint64_t*)(*pdpt_entry & ~0xFFF))[pdpt_index];
    if (!(*pd_entry & PAGE_PRESENT)) {
        *pd_entry = (uint64_t)pmm.alloc_page() | PAGE_PRESENT | PAGE_WRITE;
        memset((void*)(*pd_entry & ~0xFFF), 0, PAGE_SIZE);
    }

    uint64_t* pt_entry = &((uint64_t*)(*pd_entry & ~0xFFF))[pd_index];
    if (!(*pt_entry & PAGE_PRESENT)) {
        *pt_entry = (uint64_t)pmm.alloc_page() | PAGE_PRESENT | PAGE_WRITE;
        memset((void*)(*pt_entry & ~0xFFF), 0, PAGE_SIZE);
    }

    uint64_t* page_entry = &((uint64_t*)(*pt_entry & ~0xFFF))[pt_index];
    *page_entry = phys_addr | PAGE_PRESENT | PAGE_WRITE;
    // kprintf("Mapped phys %p to virt %p\n", (void*)phys_addr, (void*)virt_addr);
}

void Paging::load_paging() {
    uint64_t pml4_addr = (uint64_t)pml4;
    kprintf("Loading paging with PML4 address: %p\n", pml4);
    __asm__ __volatile__("mov %0, %%cr3" : : "r"(pml4_addr) : "memory");

    uint64_t cr3;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(cr3));
    kprintf("CR3 loaded with address: %p\n", (void*)cr3);
}

void Paging::enable_paging() {
    uint64_t cr0, cr4;

    // Read initial CR0 and CR4 values
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    __asm__ __volatile__("mov %%cr4, %0" : "=r"(cr4));

    kprintf("Initial CR0: %p\n", (void*)cr0);
    kprintf("Initial CR4: %p\n", (void*)cr4);

    // Enable paging in CR4 and CR0
    cr4 |= (1 << 5); // Enable PAE (Physical Address Extension)
    __asm__ __volatile__("mov %0, %%cr4" : : "r"(cr4) : "memory");

    cr0 |= (1 << 31); // Enable Paging
    __asm__ __volatile__("mov %0, %%cr0" : : "r"(cr0) : "memory");

    // Read and print CR0 and CR4 values after modification
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    __asm__ __volatile__("mov %%cr4, %0" : "=r"(cr4));

    kprintf("Modified CR0: %p\n", (void*)cr0);
    kprintf("Modified CR4: %p\n", (void*)cr4);

    kprintf("Paging enabled\n");
}
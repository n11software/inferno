#pragma once
#include <Memory/Memory.hpp>
#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096
#define PAGE_PRESENT 0x1
#define PAGE_WRITE 0x2

class PhysicalMemoryManager {
public:
    PhysicalMemoryManager() {}
    PhysicalMemoryManager(uint64_t* bitmap, uint64_t total_memory);

    void* alloc_page();
    void free_page(void* page);
    void lock_address(void* address);
	void lock_addresses(void* address, uint64_t size);

private:
    uint64_t* memory_bitmap;
    uint64_t total_pages;
};

class Paging {
public:
    Paging(PhysicalMemoryManager& pmm);

    void load_paging();
    void enable_paging();
    void map_page(uint64_t phys_addr, uint64_t virt_addr);
    uint64_t* pml4;
    uint64_t* pdpt;
    uint64_t* pd;
    uint64_t* pt;

private:
    PhysicalMemoryManager& pmm;
};
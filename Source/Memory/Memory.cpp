#include <Memory/Memory.hpp>
#include <Inferno/stdint.h>
#include <Inferno/Log.h>

extern unsigned long long _InfernoStart;
extern unsigned long long _InfernoEnd;

#define PAGES_PER_TABLE 512
#define PAGE_SIZE 4096
#define BITMAP_START 0x200000  // Use 2MB mark for bitmap

namespace Memory {
    static uint8_t* memoryBitmap = nullptr;
    static uint64_t bitmapSize = 0;
    static uint64_t lastAllocatedPage = 0;
    static uint64_t totalPages = 0;
    static uint64_t usedPages = 0;

    void Initialize(void* bitmap, uint64_t size) {
        memoryBitmap = (uint8_t*)BITMAP_START;
        bitmapSize = size;
        totalPages = (bitmapSize * 8);
        usedPages = 0;

        // Reserve first 1MB (critical CPU/BIOS area)
        uint64_t reserveLowMem = (1024 * 1024) / PAGE_SIZE;
        for (uint64_t i = 0; i < reserveLowMem / 8; i++) {
            memoryBitmap[i] = 0xFF;
            usedPages += 8;
        }

        // Reserve memory around the bitmap area
        uint64_t bitmapPages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
        uint64_t bitmapStartPage = BITMAP_START / PAGE_SIZE;
        for (uint64_t i = 0; i < bitmapPages; i++) {
            uint64_t pageIndex = bitmapStartPage + i;
            uint64_t byteIndex = pageIndex / 8;
            uint8_t bitIndex = pageIndex % 8;
            memoryBitmap[byteIndex] |= (1 << bitIndex);
            usedPages++;
        }

        // Reserve paging tables area (3MB - 3MB + 16KB)
        uint64_t pagingTableStart = 0x300000 / PAGE_SIZE;
        uint64_t pagingTablePages = 4;  // 4 pages for tables
        for (uint64_t i = 0; i < pagingTablePages; i++) {
            uint64_t pageIndex = pagingTableStart + i;
            uint64_t byteIndex = pageIndex / 8;
            uint8_t bitIndex = pageIndex % 8;
            memoryBitmap[byteIndex] |= (1 << bitIndex);
            usedPages++;
        }

        // Start allocating after bitmap
        lastAllocatedPage = (BITMAP_START / PAGE_SIZE) + bitmapPages + 1;

        prInfo("memory", "protected low memory: 0-0x%x", reserveLowMem * PAGE_SIZE);
        prInfo("memory", "bitmap at 0x%x, size: %d bytes", (uint64_t)memoryBitmap, size);
        prInfo("memory", "next free page at 0x%x", lastAllocatedPage * PAGE_SIZE);
    }

    void* RequestPage() {
        if (usedPages >= totalPages) {
            prErr("memory", "OUT OF MEMORY! used=%d total=%d", usedPages, totalPages);
            return nullptr;
        }

        void* page = nullptr;
        for (uint64_t i = lastAllocatedPage; i < totalPages; i++) {
            uint64_t byteIndex = i / 8;
            uint8_t bitIndex = i % 8;

            if (!(memoryBitmap[byteIndex] & (1 << bitIndex))) {
                memoryBitmap[byteIndex] |= (1 << bitIndex);
                page = (void*)(i * PAGE_SIZE);
                lastAllocatedPage = i + 1;
                usedPages++;

                // Validate page address
                if ((uint64_t)page < 0x1000 || (uint64_t)page >= 0x100000000ULL) {
                    prErr("memory", "invalid page address: 0x%x", (uint64_t)page);
                    return nullptr;
                }

                // Zero the page
                uint8_t* ptr = (uint8_t*)page;
                for (int j = 0; j < PAGE_SIZE; j++) {
                    ptr[j] = 0;
                }

                prInfo("memory", "allocated page 0x%x (used=%d/%d)", (uint64_t)page, usedPages, totalPages);
                return page;
            }
        }

        prErr("memory", "no free pages! (used=%d/%d)", usedPages, totalPages);
        return nullptr;
    }

    void FreePage(void* address) {
        uint64_t pageIndex = (uint64_t)address / 4096;
        uint64_t byteIndex = pageIndex / 8;
        uint8_t bitIndex = pageIndex % 8;

        memoryBitmap[byteIndex] &= ~(1 << bitIndex);
    }
}

#pragma once
#include <Inferno/stdint.h>

namespace Paging {
    struct PageTable {
        uint64_t entries[512];
    } __attribute__((aligned(4096)));

    void Initialize();
    bool IsEnabled();
    void MapPage(uint64_t virtual_addr, uint64_t physical_addr);
}

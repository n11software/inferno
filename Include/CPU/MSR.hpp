#pragma once
#include <Inferno/stdint.h>

namespace CPU {
    uint64_t ReadMSR(uint32_t msr);
    void WriteMSR(uint32_t msr, uint64_t value);
}

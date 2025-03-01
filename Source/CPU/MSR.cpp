#include <CPU/MSR.hpp>

namespace CPU {
    uint64_t ReadMSR(uint32_t msr) {
        uint32_t low, high;
        asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
        return ((uint64_t)high << 32) | low;
    }

    void WriteMSR(uint32_t msr, uint64_t value) {
        uint32_t low = value & 0xFFFFFFFF;
        uint32_t high = value >> 32;
        asm volatile("wrmsr" :: "a"(low), "d"(high), "c"(msr));
    }
}

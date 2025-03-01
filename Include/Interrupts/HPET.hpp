#pragma once

#include <Inferno/stdint.h>

namespace HPET {
    struct HPETRegisters {
        uint64_t capabilities;          // General Capabilities and ID Register
        uint64_t reserved1;
        uint64_t configuration;         // General Configuration Register
        uint64_t reserved2;
        uint64_t interrupt_status;      // General Interrupt Status Register
        uint64_t reserved3[25];
        uint64_t counter_value;         // Main Counter Value Register
        uint64_t reserved4;
    } __attribute__((packed));

    bool Initialize();
    void Enable();
    void Disable();
    void SetOneShot(uint64_t microseconds);
    void Wait(uint64_t microseconds);
    uint64_t GetFrequency();
    uint64_t GetCounterValue();
    bool IsInitialized();
}

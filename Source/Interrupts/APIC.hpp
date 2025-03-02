#ifndef _KERNEL_APIC_HPP
#define _KERNEL_APIC_HPP

#include <stdint.h>
#include <Interrupts/PIC.hpp>

namespace APIC {
    bool Capable();
    void SetBase(unsigned int base);
    unsigned int GetBase();
    void Write(unsigned int reg, unsigned int value);
    unsigned int Read(unsigned int reg);
    void Enable();
    void DisablePIC();
    bool Initialize();
    void SetupLocalAPIC();
    void ConfigureSpuriousInterrupts();
    void ConfigureTimer(uint32_t divisor);
    void SetTimerCount(uint32_t count);
    void MapIRQ(uint8_t irq, uint8_t vector);
    bool IsEnabled();
    uint32_t GetVersion();
}

#endif // _KERNEL_APIC_HPP

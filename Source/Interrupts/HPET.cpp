#include <Interrupts/HPET.hpp>
#include <Inferno/Log.h>
#include <Drivers/ACPI/acpi.h>

namespace HPET {
    static volatile HPETRegisters* hpet = nullptr;
    static uint64_t hpet_period = 0;    // Femtoseconds per tick
    static bool initialized = false;

    bool Initialize() {
        // Get HPET table from ACPI
        void* hpet_table = ACPI::FindTable("HPET");
        if (!hpet_table) {
            prErr("hpet", "HPET table not found in ACPI");
            return false;
        }

        // Map HPET MMIO space
        hpet = (HPETRegisters*)((ACPI::HPETTable*)hpet_table)->address.address;
        if (!hpet) {
            prErr("hpet", "Failed to map HPET registers");
            return false;
        }

        // Get the period (in femtoseconds)
        hpet_period = (hpet->capabilities >> 32) & 0xFFFFFFFF;
        
        // Disable legacy replacement if enabled
        hpet->configuration &= ~(1ULL << 1);
        
        // Reset main counter
        hpet->counter_value = 0;

        initialized = true;
        prInfo("hpet", "HPET initialized, period: %d fs", hpet_period);
        return true;
    }

    void Enable() {
        if (!initialized) return;
        hpet->configuration |= 1;  // Set enable bit
        prInfo("hpet", "HPET enabled");
    }

    void Disable() {
        if (!initialized) return;
        hpet->configuration &= ~1ULL;  // Clear enable bit
    }

    uint64_t GetFrequency() {
        if (!initialized) return 0;
        // Convert femtoseconds to frequency in Hz
        return 1000000000000000ULL / hpet_period;
    }

    uint64_t GetCounterValue() {
        if (!initialized) return 0;
        return hpet->counter_value;
    }

    void Wait(uint64_t microseconds) {
        if (!initialized) return;

        uint64_t ticks = (microseconds * 1000000000000ULL) / hpet_period;
        uint64_t target = GetCounterValue() + ticks;

        while (GetCounterValue() < target) {
            asm volatile("pause");
        }
    }

    bool IsInitialized() {
        return initialized;
    }
}

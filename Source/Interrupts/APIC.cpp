#include <Interrupts/APIC.hpp>
#include <CPU/CPUID.h>
#include <CPU/MSR.hpp>
#include <Inferno/IO.h>
#include <Inferno/Log.h>

// APIC Register offsets
#define APIC_ID                  0x20
#define APIC_VERSION             0x30
#define APIC_SPURIOUS            0xF0
#define APIC_EOI                 0xB0
#define APIC_ICR_LOW            0x300
#define APIC_ICR_HIGH           0x310
#define APIC_LVT_TIMER          0x320
#define APIC_TIMER_INITIAL      0x380
#define APIC_TIMER_CURRENT      0x390
#define APIC_TIMER_DIVIDE       0x3E0

namespace APIC {
	bool Capable() {
		unsigned long eax, unused, edx;
		cpuid(1, eax, unused, unused, edx);
		return edx & 1 << 9;
	}

	void SetBase(unsigned int base) {
		unsigned int edx = 0;
		unsigned int eax = (base & 0xfffff0000) | 0x800;
		#ifdef __PHYSICAL_MEMORY_EXTENSION__
			edx = (base >> 32) & 0x0f;
		#endif
		asm volatile("wrmsr" :: "a"(0x1B), "d"(eax), "c"(edx));
	}

	unsigned int GetBase() {
		unsigned int eax, edx;
		asm volatile("wrmsr" :: "a"(0x1B), "d"(eax), "c"(edx));
		#ifdef __PHYSICAL_MEMORY_EXTENSION__
			return (eax & 0xfffff000) | ((edx & 0x0f) << 32);
		#else
			return (eax & 0xfffff000);
		#endif
	}

	void Write(unsigned int reg, unsigned int value) {
		unsigned int volatile* apic = (unsigned int volatile*)GetBase();
		apic[0] = (reg & 0xff) << 4;
		apic[4] = value;
	}

	unsigned int Read(unsigned int reg) {
		unsigned int volatile* apic = (unsigned int volatile*)GetBase();
		apic[0] = (reg & 0xff) << 4;
		return apic[4];
	}

	void Enable() {
		SetBase(GetBase());
		Write(0xF0, Read(0xF0) | 0x100);
	}

	void DisablePIC() {
        prInfo("apic", "Disabling legacy PIC...");
        // Mask all interrupts
        outb(0xA1, 0xFF);
        outb(0x21, 0xFF);
        
        // Start initialization sequence
        outb(0x20, 0x11);
        outb(0xA0, 0x11);
        io_wait();
        
        // Remap PIC vectors out of the way
        outb(0x21, 0xE0);    // Master PIC vector offset
        outb(0xA1, 0xE8);    // Slave PIC vector offset
        io_wait();
        
        // Tell Master PIC there is a slave PIC at IRQ2
        outb(0x21, 0x04);
        // Tell Slave PIC its cascade identity
        outb(0xA1, 0x02);
        io_wait();
        
        // Set 8086 mode
        outb(0x21, 0x01);
        outb(0xA1, 0x01);
        io_wait();
        
        // Mask all interrupts again
        outb(0xA1, 0xFF);
        outb(0x21, 0xFF);
        
        prInfo("apic", "Legacy PIC disabled");
    }

    bool Initialize() {
        if (!Capable()) {
            prErr("apic", "APIC not supported by CPU");
            return false;
        }

        DisablePIC();
        
        // Enable APIC in MSR
		prDebug("apic", "Enabling APIC in MSR");
        uint64_t msr = CPU::ReadMSR(0x1B);
        msr |= (1 << 11);    // Enable APIC
		prDebug("apic", "Writing to MSR");
        CPU::WriteMSR(0x1B, msr);
        
		prDebug("apic", "Setup LOAPIC");
        // Set up Local APIC
        SetupLocalAPIC();
        
		prDebug("apic", "Setup SIPs");
        // Configure spurious interrupts
        ConfigureSpuriousInterrupts();
        
        prInfo("apic", "APIC initialized successfully");
        return true;
    }

    void SetupLocalAPIC() {
        // Set base address (if not already set by BIOS)
        SetBase(0xFEE00000);
        
        // Initialize all LVT entries
        Write(0x320, 0x10000);    // Timer
        Write(0x330, 0x10000);    // Thermal
        Write(0x340, 0x10000);    // Performance Counter
        Write(0x350, 0x10000);    // LINT0
        Write(0x360, 0x10000);    // LINT1
        Write(0x370, 0x10000);    // Error
    }

    void ConfigureSpuriousInterrupts() {
        // Enable spurious interrupts and set vector
        Write(APIC_SPURIOUS, Read(APIC_SPURIOUS) | 0x1FF);
    }

    void ConfigureTimer(uint32_t divisor) {
        // Set up timer divide value
        Write(APIC_TIMER_DIVIDE, divisor & 0xB);
    }

    void SetTimerCount(uint32_t count) {
        Write(APIC_TIMER_INITIAL, count);
    }

    void MapIRQ(uint8_t irq, uint8_t vector) {
        // This would involve I/O APIC configuration
        // Implementation depends on your I/O APIC setup
    }

    bool IsEnabled() {
        return (Read(APIC_SPURIOUS) & 0x100) != 0;
    }

    uint32_t GetVersion() {
        return Read(APIC_VERSION) & 0xFF;
    }
}

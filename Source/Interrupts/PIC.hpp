#ifndef _KERNEL_PIC_HPP
#define _KERNEL_PIC_HPP

#include <Inferno/IO.h>
#include <stdint.h>

// PIC ports
#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

// PIC commands
#define PIC_EOI         0x20
#define PIC_INIT        0x11

namespace PIC {
    // Send EOI to the PIC(s)
    static inline void SendEOI(uint8_t irq) {
        if (irq >= 8)
            outb(PIC2_COMMAND, PIC_EOI);
        outb(PIC1_COMMAND, PIC_EOI);
    }

    // Mask a specific IRQ line
    static inline void MaskIRQ(uint8_t irq) {
        uint16_t port;
        uint8_t value;
        
        if (irq < 8) {
            port = PIC1_DATA;
        } else {
            port = PIC2_DATA;
            irq -= 8;
        }
        
        value = inb(port) | (1 << irq);
        outb(port, value);
    }

    // Unmask a specific IRQ line
    static inline void UnmaskIRQ(uint8_t irq) {
        uint16_t port;
        uint8_t value;
        
        if (irq < 8) {
            port = PIC1_DATA;
        } else {
            port = PIC2_DATA;
            irq -= 8;
        }
        
        value = inb(port) & ~(1 << irq);
        outb(port, value);
    }

    // Disable (mask) all PIC interrupts 
    static inline void Disable() {
        // Mask all interrupts
        outb(PIC1_DATA, 0xFF);
        io_wait();
        outb(PIC2_DATA, 0xFF);
        io_wait();
    }

    // Perform a full PIC remapping safely
    static inline bool Remap(uint8_t offset1, uint8_t offset2) {
        // Save masks
        uint8_t mask1 = inb(PIC1_DATA);
        uint8_t mask2 = inb(PIC2_DATA);
        
        // Start initialization sequence in cascade mode
        outb(PIC1_COMMAND, PIC_INIT);
        io_wait();
        outb(PIC2_COMMAND, PIC_INIT);
        io_wait();
        
        // Set vector offsets
        outb(PIC1_DATA, offset1);
        io_wait();
        outb(PIC2_DATA, offset2);
        io_wait();
        
        // Continue initialization sequence
        outb(PIC1_DATA, 4);  // Tell Master PIC that there is a slave at IRQ2
        io_wait();
        outb(PIC2_DATA, 2);  // Tell Slave PIC its cascade identity
        io_wait();
        
        // Set 8086 mode
        outb(PIC1_DATA, 0x01);
        io_wait();
        outb(PIC2_DATA, 0x01);
        io_wait();
        
        // Restore masks
        outb(PIC1_DATA, mask1);
        io_wait();
        outb(PIC2_DATA, mask2);
        io_wait();
        
        return true;
    }
}

#endif // _KERNEL_PIC_HPP

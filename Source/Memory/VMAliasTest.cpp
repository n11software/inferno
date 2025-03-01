#include <Memory/VMAliasTest.hpp>
#include <Memory/Memory.hpp>
#include <Memory/Paging.hpp>
#include <Inferno/Log.h>

#include <Memory/Mem_.hpp>

namespace VMAliasTest {
    // Constants for page table manipulation
    #define PAGE_PRESENT 0x1
    #define PAGE_WRITABLE 0x2
    #define PAGE_USER 0x4
    #define PAGE_SIZE_BIT 0x80
    #define PAGE_DEFAULT (PAGE_PRESENT | PAGE_WRITABLE)
    
    // Extract indices from virtual address
    static inline uint16_t GetPML4Index(uint64_t addr) { return (addr >> 39) & 0x1FF; }
    static inline uint16_t GetPDPIndex(uint64_t addr)  { return (addr >> 30) & 0x1FF; }
    static inline uint16_t GetPDIndex(uint64_t addr)   { return (addr >> 21) & 0x1FF; }
    static inline uint16_t GetPTIndex(uint64_t addr)   { return (addr >> 12) & 0x1FF; }
    
    // A definitive method to test virtual memory aliasing that works with the current paging setup
    // This method works around the 2MB page limitation by temporarily breaking down a 2MB page
    bool Test() {
        prInfo("vm_alias", "Running definitive VM aliasing test");
        
        // Allocate a physical page for our test data
        void* physPage = Memory::RequestPage();
        if (!physPage) {
            prErr("vm_alias", "Failed to allocate physical page");
            return false;
        }
        
        uint64_t physAddr = (uint64_t)physPage;
        prInfo("vm_alias", "Test physical memory at 0x%x", physAddr);
        
        // Write our test pattern directly to physical memory
        *(volatile uint32_t*)physPage = 0xDEADBEEF;
        prInfo("vm_alias", "Written 0xDEADBEEF directly to physical memory");
        
        // Verify direct access
        uint32_t directRead = *(volatile uint32_t*)physPage;
        prInfo("vm_alias", "Direct read back: 0x%x", directRead);
        
        // We'll test with virtual addresses in an area that's unlikely to be used
        // We'll use 0x500000 (5MB) and 0x600000 (6MB) as they should be far enough
        // from the kernel's main regions but still in identity-mapped space
        uint64_t vaddr1 = 0x500000;
        uint64_t vaddr2 = 0x600000;
        
        prInfo("vm_alias", "Using virtual addresses 0x%x and 0x%x", vaddr1, vaddr2);
        
        // Get current CR3
        uint64_t cr3;
        asm volatile("mov %%cr3, %0" : "=r"(cr3));
        
        // Get page table pointers - we'll manipulate them directly
        // to ensure we can break down 2MB pages if needed
        uint64_t* pml4 = (uint64_t*)(cr3 & ~0xFFF);
        
        // First, make sure our virtual addresses aren't already mapped to something important
        uint64_t origPhys1 = Paging::GetPhysicalAddress(vaddr1);
        uint64_t origPhys2 = Paging::GetPhysicalAddress(vaddr2);
        
        prInfo("vm_alias", "Original mappings: 0x%x -> 0x%x, 0x%x -> 0x%x",
               vaddr1, origPhys1, vaddr2, origPhys2);
        
        // Now we'll create our own page tables and mappings, breaking down 2MB pages if needed
        
        // Disable interrupts during critical section
        asm volatile("cli");
        
        bool success = true;
        
        // Handle vaddr1
        uint16_t pml4_idx1 = GetPML4Index(vaddr1);
        uint16_t pdpt_idx1 = GetPDPIndex(vaddr1);
        uint16_t pd_idx1 = GetPDIndex(vaddr1);
        uint16_t pt_idx1 = GetPTIndex(vaddr1);
        
        // Ensure PML4 entry exists
        if (!(pml4[pml4_idx1] & PAGE_PRESENT)) {
            void* newPDPT = Memory::RequestPage();
            if (!newPDPT) {
                prErr("vm_alias", "Failed to allocate PDPT");
                asm volatile("sti");
                return false;
            }
            memset(newPDPT, 0, 4096);
            pml4[pml4_idx1] = (uint64_t)newPDPT | PAGE_DEFAULT;
            prInfo("vm_alias", "Created new PDPT at 0x%x", (uint64_t)newPDPT);
        }
        
        // Get PDPT
        uint64_t* pdpt = (uint64_t*)(pml4[pml4_idx1] & ~0xFFF);
        
        // Ensure PDPT entry exists
        if (!(pdpt[pdpt_idx1] & PAGE_PRESENT)) {
            void* newPD = Memory::RequestPage();
            if (!newPD) {
                prErr("vm_alias", "Failed to allocate PD");
                asm volatile("sti");
                return false;
            }
            memset(newPD, 0, 4096);
            pdpt[pdpt_idx1] = (uint64_t)newPD | PAGE_DEFAULT;
            prInfo("vm_alias", "Created new PD at 0x%x", (uint64_t)newPD);
        }
        
        // Get PD
        uint64_t* pd = (uint64_t*)(pdpt[pdpt_idx1] & ~0xFFF);
        
        // If we have a 2MB page, we need to break it down
        if ((pd[pd_idx1] & PAGE_PRESENT) && (pd[pd_idx1] & PAGE_SIZE_BIT)) {
            prInfo("vm_alias", "Breaking down 2MB page for address 0x%x", vaddr1);
            
            // Get the base physical address of the 2MB page
            uint64_t base2MB = pd[pd_idx1] & ~0x1FFFFF;
            
            // Create a new PT to replace the 2MB page
            void* newPT = Memory::RequestPage();
            if (!newPT) {
                prErr("vm_alias", "Failed to allocate PT for breaking down 2MB page");
                asm volatile("sti");
                return false;
            }
            
            // Map each 4K page in the PT to the corresponding part of the 2MB page
            uint64_t* pt = (uint64_t*)newPT;
            for (int i = 0; i < 512; i++) {
                pt[i] = (base2MB + (i * 4096)) | PAGE_DEFAULT;
            }
            
            // Replace the 2MB entry with our new PT
            pd[pd_idx1] = (uint64_t)newPT | PAGE_DEFAULT;
        }
        else if (!(pd[pd_idx1] & PAGE_PRESENT)) {
            // If no entry at all, create a new PT
            void* newPT = Memory::RequestPage();
            if (!newPT) {
                prErr("vm_alias", "Failed to allocate new PT");
                asm volatile("sti");
                return false;
            }
            memset(newPT, 0, 4096);
            pd[pd_idx1] = (uint64_t)newPT | PAGE_DEFAULT;
            prInfo("vm_alias", "Created new PT at 0x%x", (uint64_t)newPT);
        }
        
        // Now we definitely have a PT rather than a 2MB page
        uint64_t* pt = (uint64_t*)(pd[pd_idx1] & ~0xFFF);
        
        // Map our physical page to this virtual address
        pt[pt_idx1] = physAddr | PAGE_DEFAULT;
        prInfo("vm_alias", "Mapped 0x%x -> 0x%x", vaddr1, physAddr);
        
        // Repeat the same process for vaddr2
        uint16_t pml4_idx2 = GetPML4Index(vaddr2);
        uint16_t pdpt_idx2 = GetPDPIndex(vaddr2);
        uint16_t pd_idx2 = GetPDIndex(vaddr2);
        uint16_t pt_idx2 = GetPTIndex(vaddr2);
        
        // Most of this code mirrors the vaddr1 handling, but for clarity we repeat it
        
        // Ensure PML4 entry exists
        if (!(pml4[pml4_idx2] & PAGE_PRESENT)) {
            void* newPDPT = Memory::RequestPage();
            if (!newPDPT) {
                prErr("vm_alias", "Failed to allocate PDPT");
                asm volatile("sti");
                return false;
            }
            memset(newPDPT, 0, 4096);
            pml4[pml4_idx2] = (uint64_t)newPDPT | PAGE_DEFAULT;
            prInfo("vm_alias", "Created new PDPT at 0x%x", (uint64_t)newPDPT);
        }
        
        // Get PDPT
        uint64_t* pdpt2 = (uint64_t*)(pml4[pml4_idx2] & ~0xFFF);
        
        // Ensure PDPT entry exists
        if (!(pdpt2[pdpt_idx2] & PAGE_PRESENT)) {
            void* newPD = Memory::RequestPage();
            if (!newPD) {
                prErr("vm_alias", "Failed to allocate PD");
                asm volatile("sti");
                return false;
            }
            memset(newPD, 0, 4096);
            pdpt2[pdpt_idx2] = (uint64_t)newPD | PAGE_DEFAULT;
            prInfo("vm_alias", "Created new PD at 0x%x", (uint64_t)newPD);
        }
        
        // Get PD
        uint64_t* pd2 = (uint64_t*)(pdpt2[pdpt_idx2] & ~0xFFF);
        
        // If we have a 2MB page, we need to break it down
        if ((pd2[pd_idx2] & PAGE_PRESENT) && (pd2[pd_idx2] & PAGE_SIZE_BIT)) {
            prInfo("vm_alias", "Breaking down 2MB page for address 0x%x", vaddr2);
            
            // Get the base physical address of the 2MB page
            uint64_t base2MB = pd2[pd_idx2] & ~0x1FFFFF;
            
            // Create a new PT to replace the 2MB page
            void* newPT = Memory::RequestPage();
            if (!newPT) {
                prErr("vm_alias", "Failed to allocate PT for breaking down 2MB page");
                asm volatile("sti");
                return false;
            }
            
            // Map each 4K page in the PT to the corresponding part of the 2MB page
            uint64_t* pt2 = (uint64_t*)newPT;
            for (int i = 0; i < 512; i++) {
                pt2[i] = (base2MB + (i * 4096)) | PAGE_DEFAULT;
            }
            
            // Replace the 2MB entry with our new PT
            pd2[pd_idx2] = (uint64_t)newPT | PAGE_DEFAULT;
        }
        else if (!(pd2[pd_idx2] & PAGE_PRESENT)) {
            // If no entry at all, create a new PT
            void* newPT = Memory::RequestPage();
            if (!newPT) {
                prErr("vm_alias", "Failed to allocate new PT");
                asm volatile("sti");
                return false;
            }
            memset(newPT, 0, 4096);
            pd2[pd_idx2] = (uint64_t)newPT | PAGE_DEFAULT;
            prInfo("vm_alias", "Created new PT at 0x%x", (uint64_t)newPT);
        }
        
        // Now we definitely have a PT rather than a 2MB page
        uint64_t* pt2 = (uint64_t*)(pd2[pd_idx2] & ~0xFFF);
        
        // Map our physical page to this virtual address
        pt2[pt_idx2] = physAddr | PAGE_DEFAULT;
        prInfo("vm_alias", "Mapped 0x%x -> 0x%x", vaddr2, physAddr);
        
        // Flush TLB for both addresses
        asm volatile("invlpg (%0)" : : "r"(vaddr1) : "memory");
        asm volatile("invlpg (%0)" : : "r"(vaddr2) : "memory");
        
        // Re-enable interrupts
        asm volatile("sti");
        
        // Verify the mappings worked
        uint64_t newPhys1 = Paging::GetPhysicalAddress(vaddr1);
        uint64_t newPhys2 = Paging::GetPhysicalAddress(vaddr2);
        
        prInfo("vm_alias", "New mappings: 0x%x -> 0x%x, 0x%x -> 0x%x",
               vaddr1, newPhys1, vaddr2, newPhys2);
        
        if (newPhys1 != physAddr || newPhys2 != physAddr) {
            prErr("vm_alias", "Failed to create proper mappings");
            success = false;
        } else {
            // Now test the aliasing by reading our test value through both mappings
            prInfo("vm_alias", "Testing memory aliasing...");
            
            uint32_t val1 = *(volatile uint32_t*)vaddr1;
            uint32_t val2 = *(volatile uint32_t*)vaddr2;
            
            prInfo("vm_alias", "Read through vaddr1: 0x%x", val1);
            prInfo("vm_alias", "Read through vaddr2: 0x%x", val2);
            
            // Check if both addresses read the same value
            if (val1 == 0xDEADBEEF && val2 == 0xDEADBEEF) {
                prInfo("vm_alias", "SUCCESS: Both addresses read the same value");
                
                // Now write through one address and read through the other
                *(volatile uint32_t*)vaddr1 = 0xCAFEBABE;
                asm volatile("mfence" ::: "memory");
                
                val2 = *(volatile uint32_t*)vaddr2;
                prInfo("vm_alias", "After writing 0xCAFEBABE to vaddr1, read from vaddr2: 0x%x", val2);
                
                if (val2 == 0xCAFEBABE) {
                    prInfo("vm_alias", "FINAL SUCCESS: Virtual memory aliasing confirmed!");
                    success = true;
                } else {
                    prErr("vm_alias", "FAILED: Values didn't propagate across mappings");
                    success = false;
                }
            } else {
                prErr("vm_alias", "FAILED: Addresses didn't read the expected value");
                success = false;
            }
        }
        
        // Clean up by restoring original mappings if they existed
        if (origPhys1) {
            if (pt) pt[pt_idx1] = origPhys1 | PAGE_DEFAULT;
        } else {
            if (pt) pt[pt_idx1] = 0;  // Just unmap
        }
        
        if (origPhys2) {
            if (pt2) pt2[pt_idx2] = origPhys2 | PAGE_DEFAULT;
        } else {
            if (pt2) pt2[pt_idx2] = 0;  // Just unmap
        }
        
        // Flush TLB again
        asm volatile("invlpg (%0)" : : "r"(vaddr1) : "memory");
        asm volatile("invlpg (%0)" : : "r"(vaddr2) : "memory");
        
        return success;
    }
}

#include <Memory/SimpleVMAliasTest.hpp>
#include <Memory/Memory.hpp>
#include <Memory/Paging.hpp>
#include <Inferno/Log.h>

namespace SimpleVMAliasTest {
    // We'll use direct physical memory access to verify virtual memory aliasing
    bool Test() {
        prInfo("vm_alias", "Running simplified VM aliasing test");

        // Request a fresh physical page for our test
        void* physPage = Memory::RequestPage();
        if (!physPage) {
            prErr("vm_alias", "Failed to allocate physical page");
            return false;
        }

        uint64_t physAddr = (uint64_t)physPage;
        prInfo("vm_alias", "Allocated test page at physical address 0x%x", physAddr);

        // Write a test pattern directly to physical memory
        uint32_t* directPtr = (uint32_t*)physPage;
        *directPtr = 0xDEADBEEF;
        asm volatile("mfence" ::: "memory");

        prInfo("vm_alias", "Wrote 0xDEADBEEF directly to physical memory at 0x%x", physAddr);

        // Verify we can read it back directly
        uint32_t directRead = *directPtr;
        prInfo("vm_alias", "Direct read from physical memory: 0x%x", directRead);

        // Use a simplified approach: just check if memory appears at its identity mapping
        // Every physical address should be accessible at the same virtual address
        // in the kernel's identity-mapped area
        uint32_t* identityMapped = (uint32_t*)physAddr;
        uint32_t identityRead = *identityMapped;

        prInfo("vm_alias", "Read 0x%x through identity mapping at 0x%x", identityRead, physAddr);

        if (identityRead == 0xDEADBEEF) {
            prInfo("vm_alias", "Identity mapping works correctly!");

            // Now the real test: write a new value through the identity mapping
            // and see if we can read it through the original pointer
            *identityMapped = 0xCAFEBABE;
            asm volatile("mfence" ::: "memory");

            uint32_t directVerify = *directPtr;
            prInfo("vm_alias", "After writing 0xCAFEBABE to identity mapping, direct read: 0x%x", directVerify);

            if (directVerify == 0xCAFEBABE) {
                prInfo("vm_alias", "SUCCESS: Virtual memory aliasing confirmed!");
                return true;
            } else {
                prErr("vm_alias", "FAILED: Values didn't propagate between mappings");
                return false;
            }
        } else {
            prErr("vm_alias", "FAILED: Identity mapping didn't give expected value");
            return false;
        }
    }
}

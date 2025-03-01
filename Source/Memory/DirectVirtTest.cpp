#include <Memory/DirectVirtTest.hpp>
#include <Memory/Memory.hpp>
#include <Memory/Paging.hpp>
#include <Inferno/Log.h>
#include <Memory/Mem_.hpp>

namespace DirectVirtTest {
    #define TEST_PATTERN 0xDEADBEEFCAFEBABEULL
    
    // A safer virtual memory mapping test that avoids manipulating page tables directly
    void TestVirtualAliasing() {
        prInfo("virt_test", "Starting safer virtual memory test");
        
        // Allocate a physical page for testing
        void* physMem = Memory::RequestPage();
        if (!physMem) {
            prErr("virt_test", "Failed to allocate test page");
            return;
        }
        
        uint64_t physAddr = (uint64_t)physMem;
        prInfo("virt_test", "Test physical memory at 0x%x", physAddr);
        
        // Clear memory to known state
        memset(physMem, 0, 4096);
        
        // Choose relatively low memory addresses that are less likely to cause issues
        // Stay below 16MB to avoid issues with large page boundaries
        uint64_t virtAddr1 = 0x800000;  // 8MB
        uint64_t virtAddr2 = 0x900000;  // 9MB
        
        prInfo("virt_test", "Using virtual addresses 0x%x and 0x%x", virtAddr1, virtAddr2);
        
        // First, make sure these regions are unmapped
        Paging::UnmapPage(virtAddr1);
        Paging::UnmapPage(virtAddr2);
        
        // Enable interrupts during most of the test
        asm volatile("sti");
        
        // Map the first virtual address to our physical memory
        prInfo("virt_test", "Mapping first virtual address 0x%x -> 0x%x", virtAddr1, physAddr);
        Paging::MapPage(virtAddr1, physAddr);
        
        // Map the second virtual address to the same physical memory
        prInfo("virt_test", "Mapping second virtual address 0x%x -> 0x%x", virtAddr2, physAddr);
        Paging::MapPage(virtAddr2, physAddr);
        
        // Force TLB flush for both addresses
        asm volatile("invlpg (%0)" : : "r"(virtAddr1) : "memory");
        asm volatile("invlpg (%0)" : : "r"(virtAddr2) : "memory");
        asm volatile("mfence" ::: "memory");
        
        // Verify the mappings with GetPhysicalAddress
        uint64_t check1 = Paging::GetPhysicalAddress(virtAddr1);
        uint64_t check2 = Paging::GetPhysicalAddress(virtAddr2);
        
        prInfo("virt_test", "Verified mappings: 0x%x -> 0x%x, 0x%x -> 0x%x", 
               virtAddr1, check1, virtAddr2, check2);
        
        if (check1 != physAddr || check2 != physAddr) {
            prErr("virt_test", "ERROR: Virtual mappings did not set up correctly");
            Paging::UnmapPage(virtAddr1);
            Paging::UnmapPage(virtAddr2);
            return;
        }
        
        // No try-catch - we'll just perform the operations directly
        
        // Write a test pattern through the first virtual address
        prInfo("virt_test", "Writing test pattern 0x%x to 0x%x", TEST_PATTERN, virtAddr1);
        
        volatile uint64_t* writePtr = (volatile uint64_t*)virtAddr1;
        *writePtr = TEST_PATTERN;
        
        // Memory barrier to ensure write completes
        asm volatile("mfence" ::: "memory");
        
        // Read through the second virtual address
        volatile uint64_t* readPtr = (volatile uint64_t*)virtAddr2;
        uint64_t readValue = *readPtr;
        
        // Compare the values
        prInfo("virt_test", "Read 0x%x from 0x%x (expected 0x%x)", 
               readValue, virtAddr2, TEST_PATTERN);
        
        if (readValue == TEST_PATTERN) {
            prInfo("virt_test", "SUCCESS: Virtual memory aliasing works correctly!");
        } else {
            prErr("virt_test", "FAILED: Virtual memory aliasing test failed");
            prErr("virt_test", "Expected 0x%x but got 0x%x", TEST_PATTERN, readValue);
        }
        
        // Clean up - unmap both virtual addresses
        Paging::UnmapPage(virtAddr1);
        Paging::UnmapPage(virtAddr2);
        
        prInfo("virt_test", "Virtual memory test completed");
    }
}

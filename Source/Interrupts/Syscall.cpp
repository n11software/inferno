//========= Copyright N11 Software, All rights reserved. ============//
//
// File: Syscall.cpp
// Purpose: Syscall Handler
// Maintainer: FiReLScar
//
//===================================================================//

#include <Drivers/TTY/COM.h>
#include <Interrupts/Syscall.hpp>
#include <Memory/Heap.hpp>
#include <Memory/Memory.hpp>
#include <Memory/Paging.hpp>
#include <Inferno/Log.h>

// Program break - used by brk/sbrk syscalls
static uint64_t current_brk = 0x4000000;  // Initial program break at 4MB

// Linux mmap implementation
// %rdi: addr, %rsi: len, %rdx: prot, %r10: flags, %r8: fd, %r9: offset
static uint64_t sys_mmap(uint64_t addr, uint64_t len, uint64_t prot, uint64_t flags, uint64_t fd, uint64_t offset) {
    prInfo("syscall", "mmap(addr=0x%x, len=%d, prot=0x%x, flags=0x%x, fd=%d, offset=0x%x)",
           addr, len, prot, flags, fd, offset);
    
    // Round up to page size
    len = (len + 0xFFF) & ~0xFFF;
    
    // If addr is 0, we choose the address
    if (addr == 0) {
        // We'll use a simple strategy for now - just use a fixed base address
        addr = 0x5000000;  // 80MB mark
    }
    
    // Map each page
    for (uint64_t map_offset = 0; map_offset < len; map_offset += 0x1000) {
        void* page = Memory::RequestPage();
        if (!page) {
            prErr("syscall", "mmap failed: out of memory");
            return -1;  // MAP_FAILED in Linux
        }
        Paging::MapPage(addr + map_offset, (uint64_t)page);
    }
    
    return addr;  // Return the address of the mapping
}

// Linux brk implementation
static uint64_t sys_brk(uint64_t new_brk, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    prInfo("syscall", "brk(0x%x)", new_brk);
    
    // If new_brk is 0, just return current brk
    if (new_brk == 0) {
        return current_brk;
    }
    
    // Otherwise, update brk
    current_brk = new_brk;
    return current_brk;
}

// Linux syscall handlers
static syscall_handler_t syscall_table[] = {
    nullptr,    // SYS_READ
    nullptr,    // SYS_WRITE
    nullptr,    // SYS_OPEN
    nullptr,    // SYS_CLOSE
    nullptr,    // SYS_STAT
    nullptr,    // SYS_FSTAT
    nullptr,    // SYS_LSTAT
    nullptr,    // SYS_POLL
    nullptr,    // SYS_LSEEK
    sys_mmap,   // SYS_MMAP
    nullptr,    // SYS_MPROTECT
    nullptr,    // SYS_MUNMAP
    sys_brk,    // SYS_BRK
};

// Interrupt handler for syscalls
__attribute__((interrupt)) void SyscallHandler(void* frame) {
    // Extract syscall number and arguments from Linux convention registers
    uint64_t syscall_num, arg1, arg2, arg3, arg4, arg5, arg6;

    // Get registers following Linux syscall convention:
    // %rax: syscall number
    // %rdi: arg1, %rsi: arg2, %rdx: arg3, %r10: arg4, %r8: arg5, %r9: arg6
    asm volatile(
        "mov %%rax, %0\n"
        "mov %%rdi, %1\n"
        "mov %%rsi, %2\n"
        "mov %%rdx, %3\n"
        "mov %%r10, %4\n"
        "mov %%r8,  %5\n"
        "mov %%r9,  %6\n"
        : "=r"(syscall_num), "=r"(arg1), "=r"(arg2), "=r"(arg3), 
          "=r"(arg4), "=r"(arg5), "=r"(arg6)
        :
        : "memory"
    );

    prInfo("syscall", "Syscall number: %d (args: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x)",
           syscall_num, arg1, arg2, arg3, arg4, arg5, arg6);

    // Default return value
    uint64_t result = -1;

    // Call appropriate handler from syscall table
    if (syscall_num < sizeof(syscall_table) / sizeof(syscall_handler_t) && 
        syscall_table[syscall_num] != nullptr) {
        result = syscall_table[syscall_num](arg1, arg2, arg3, arg4, arg5, arg6);
    } else {
        prErr("syscall", "Invalid syscall number: %d", syscall_num);
    }

    // Set return value in RAX per Linux convention
    asm volatile("mov %0, %%rax" : : "r"(result) : "memory");

    // The compiler will automatically generate the proper iretq sequence
}

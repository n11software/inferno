//========= Copyright N11 Software, All rights reserved. ============//
//
// File: Syscall.hpp
// Purpose: Syscall handler
// Maintainer: FiReLScar
//
//===================================================================//

#pragma once
#include <Inferno/stdint.h>

// Linux syscall numbers for x86_64
#define SYS_READ      0
#define SYS_WRITE     1
#define SYS_OPEN      2
#define SYS_CLOSE     3
#define SYS_STAT      4
#define SYS_FSTAT     5
#define SYS_LSTAT     6
#define SYS_POLL      7
#define SYS_LSEEK     8
#define SYS_MMAP      9
#define SYS_MPROTECT  10
#define SYS_MUNMAP    11
#define SYS_BRK       12

typedef uint64_t (*syscall_handler_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

extern "C" {
    void SyscallHandler(void*);
}
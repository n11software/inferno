//========= Copyright N11 Software, All rights reserved. ============//
//
// File: Syscall.hpp
// Purpose: Syscall handler
// Maintainer: FiReLScar
//
//===================================================================//

#pragma once
#include <Inferno/stdint.h>

#define SYS_WRITE 1
#define SYS_MALLOC 2
#define SYS_FREE 3

typedef uint64_t (*syscall_handler_t)(uint64_t arg1, uint64_t arg2);
__attribute__((interrupt)) void SyscallHandler(void* frame);
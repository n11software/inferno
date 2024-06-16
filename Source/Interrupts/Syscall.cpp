//========= Copyright N11 Software, All rights reserved. ============//
//
// File: Syscall.cpp
// Purpose: Syscall Handler
// Maintainer: FiReLScar
//
//===================================================================//

#include <Drivers/TTY/COM.h>
#include <Interrupts/Syscall.hpp>

void SyscallHandler(void*) {
    register unsigned long func asm("rax");
    switch (func) {
        case 1:
            register unsigned int descriptor asm("rdi");
            register const char* buf asm("rdx");
            kprintf(buf);
            break;
    }
}

//========= Copyright N11 Software, All rights reserved. ============//
//
// File: PageFault.cpp
// Purpose:
// Maintainer: FiReLScar
//
//===================================================================//

#include <Drivers/TTY/COM.h>
#include <Interrupts/PageFault.hpp>

void PageFault(void*) {
    kprintf("\r\e[31m[ERROR] Page Fault\e[0m\n\r");
    while(1) asm("hlt");
}

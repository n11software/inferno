//========= Copyright N11 Software, All rights reserved. ============//
//
// File: IO.cpp
// Purpose:
// Maintainer: aristonl, FiReLScar
//
//===================================================================//

#include <Inferno/IO.h>

void outb(unsigned short port, unsigned char value) {
    asm volatile("outb %0, %1" :: "a"(value), "Nd"(port));
}

unsigned char inb(unsigned short port) {
    unsigned char returnVal;
    asm volatile("inb %1, %0" : "=a"(returnVal) : "Nd"(port));
    return returnVal;
}

void outl(unsigned short port, unsigned int value) {
    asm volatile("outl %0, %1" :: "a"(value), "Nd"(port));
}

unsigned int inl(unsigned short port) {
    unsigned int returnVal;
    asm volatile("inl %1, %0" : "=a"(returnVal) : "Nd"(port));
    return returnVal;
}

void io_wait() {
    asm volatile("outb %%al, $0x80" ::"a"(0));
}

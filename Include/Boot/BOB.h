//========= Copyright N11 Software, All rights reserved. ============//
//
// File: BOB.h
// Purpose:
// Maintainer: FiReLScar
//
//===================================================================//

#pragma once
#include <Drivers/Graphics/Framebuffer.h>

struct MemoryDescriptor {
    unsigned int Type;
    void* PhysicalStart, *VirtualStart;
    unsigned long long NumberOfPages, Attribute;
};

struct BOB {
    unsigned long long DescriptorSize, MapSize;
    MemoryDescriptor* MemoryMap;
    void* RSDP;
    Framebuffer* framebuffer;
    void* FontFile;
};

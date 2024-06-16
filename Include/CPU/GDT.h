//========= Copyright N11 Software, All rights reserved. ============//
//
// File: GDT.h
// Purpose:
// Maintainer: FiReLScar
//
//===================================================================//

#pragma once
#include <Inferno/Config.h>

#if EnableGDT == true
    namespace GDT {
        struct Descriptor {
            unsigned short size;
            unsigned long long offset;
        } __attribute__((packed));

        struct Entry {
            unsigned short limit, baseLow;
            unsigned char base, access, granularity, baseHigh;
        } __attribute__((packed));

        struct Table {
            GDT::Entry null, code, data, userspaceCode, userspaceData;
        } __attribute__((packed))
        __attribute((aligned(0x1000)));
    }

    extern "C" void LoadGDT(GDT::Descriptor* descriptor);
#endif // EnableGDT

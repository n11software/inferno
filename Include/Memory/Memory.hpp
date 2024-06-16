//========= Copyright N11 Software, All rights reserved. ============//
//
// File: Memory.hpp
// Purpose:
// Maintainer: FiReLScar
//
//===================================================================//

#pragma once
#include <Boot/BOB.h>
#include <Inferno/Log.h>
#include <Memory/Mem_.hpp>

namespace Memory {
    void Init(BOB* bob);
    unsigned long long GetSize(), GetUsed(), GetFree();
    void SetUsed(unsigned long long used);
    void SetFree(unsigned long long free);
}


class Bitmap {
    public:
        unsigned long Size;
        unsigned char* Buffer;
        bool operator[](unsigned long long index) {
            unsigned long long byteIndex = index / 8;
            unsigned char bitIndex = index % 8;
            unsigned char bitIndexer = 0b10000000 >> bitIndex;
            if ((Buffer[byteIndex] & bitIndexer) > 0) return true;
            return false;
        }

        bool Set(unsigned long long index, bool value) {
            if (index>Size*8) return false;
            unsigned long long byteIndex = index / 8;
            unsigned char bitIndex = index % 8;
            unsigned char bitIndexer = 0b10000000 >> bitIndex;
            Buffer[byteIndex] &= ~bitIndexer;
            if (value) Buffer[byteIndex] |= bitIndexer;
            return true;
        }
};
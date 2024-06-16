//========= Copyright N11 Software, All rights reserved. ============//
//
// File: CPUID.hpp
// Maintainer: aristonl, levih
//
//===================================================================//

#pragma once

namespace CPU {
    const char *VendorID();
    char *Model();
}

#define cpuid(in, a, b, c, d) __asm__ __volatile__ ("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "0"(in));
//========= Copyright N11 Software, All rights reserved. ============//
//
// File: cmos.cpp
// Purpose: CMOS related functions
// Maintainer: aristonl
//
//===================================================================//

#include <Inferno/IO.h>
#include <Drivers/CMOS/CMOS.h>

unsigned char getRegister(int reg) {
      outb(cmosAddress, reg);
      return inb(cmosData);
}

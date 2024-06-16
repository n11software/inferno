//========= Copyright N11 Software, All rights reserved. ============//
//
// File: cmos.h
// Purpose: CMOS related functions
// Maintainer: aristonl
//
//===================================================================//

#pragma once

#include <Inferno/IO.h>

enum {
      cmosAddress = 0x70,
      cmosData    = 0x71
};

unsigned char getRegister(int reg);

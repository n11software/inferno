//========= Copyright N11 Software, All rights reserved. ============//
//
// File: rtc.h
// Purpose: Main header file for all RTC drivers
// Maintainer: aristonl
//
//===================================================================//

#pragma once

namespace RTC {
void readRTC();
unsigned long long getEpochTime();
int init();
}

//========= Copyright N11 Software, All rights reserved. ============//
//
// File: log.h
// Purpose: Message logging functions
// Maintainer: aristonl
//
//===================================================================//

#pragma once

#include <Drivers/TTY/COM.h>
#include <Inferno/stdarg.h>

void prInfo(const char* subsystem, const char* message, ...);
void prErr(const char* subsystem, const char* message, ...);
void prWarn(const char* subsystem, const char* message, ...);
void prDebug(const char* subsystem, const char* message, ...);

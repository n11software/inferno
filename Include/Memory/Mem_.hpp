//========= Copyright N11 Software, All rights reserved. ============//
//
// File: Mem_.hpp
// Purpose:
// Maintainer: FiReLScar
//
//===================================================================//

#pragma once

void* memset(void* destptr, int value, unsigned long int size);
void* memcpy(void* destptr, void const* srcptr, unsigned long int size);

int memcmp(const void* ptr1, const void* ptr2, unsigned long int size);
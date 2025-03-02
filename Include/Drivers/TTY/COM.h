//========= Copyright N11 Software, All rights reserved. ============//
//
// File: COM.h
// Purpose:
// Maintainer: FiReLScar
//
//===================================================================//

#pragma once
#include <Inferno/IO.h>
#include <Drivers/Graphics/Framebuffer.h>

void initFB(Framebuffer* fb);

void InitializeSerialDevice();
char AwaitSerialResponse();
void kputchar(char a);
int kprintf(const char* fmt, ...);
void initFB(Framebuffer* fb);
void readSerial(char* buffer, int length);

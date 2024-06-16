/*
 * Copyright (c) 2018, 2019, 2021, 2022, 2023, 2024
 * 	N11 Software. All rights reserved.
 *
 * @N11_PUBLIC_SOURCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the N11 Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of software, or to circumvent, violate,
 * or enable the circumvention or violation of, any terms of a software
 * license agreement.
 *
 * Please obtain a copy of the License using Git at:
 * git clone ssh://n11.dev:23231/health-files.git
 * Navigate to the following location: npsl/N11_LICENSE_2
 * and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND N11 HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @N11_PUBLIC_SOURCE_LICENSE_HEADER_END@
 */

#pragma once

unsigned char inb(unsigned short port);
unsigned char inb_p(unsigned short port);
unsigned short inw(unsigned short port);
unsigned short inw_p(unsigned short port);
unsigned int inl(unsigned short port);
unsigned int inl_p(unsigned short port);

void outb(unsigned short port, unsigned char value);
void outb_p(unsigned char value, unsigned short port);
void outw(unsigned short value, unsigned short port);
void outw_p(unsigned short value, unsigned short port);
void outl(unsigned short port, unsigned int value);
void outl_p(unsigned int value, unsigned short port);

void insb(unsigned short port, void* addr,
    unsigned long count);
void insw(unsigned short port, void* addr,
    unsigned long count);
void insl(unsigned short port, void* addr,
    unsigned long count);
void outsb(unsigned short port, const void* addr,
    unsigned long count);
void outsw(unsigned short port, const void* addr,
    unsigned long count);
void outsl(unsigned short port, const void* addr,
    unsigned long count);

void io_wait();


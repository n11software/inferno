//========= Copyright N11 Software, All rights reserved. ============//
//
// File: brk.cpp
// Purpose: Implementations of brk() and sbrk()
// Maintainer: aristonl
//
//===================================================================//

#include <Inferno/unistd.h>
#include <Inferno/stdint.h>
#include <Inferno/errno.h>

#define __NR_brk 214


#define syscall1(num, arg1)                                                \
({                                                                            \
	long _ret;                                                            \
	register long _num  __asm__ ("rax") = (num);                          \
	register long _arg1 __asm__ ("rdi") = (long)(arg1);                   \
	                                                                      \
	__asm__  volatile (                                                   \
		"syscall\n"                                                   \
		: "=a"(_ret)                                                  \
		: "r"(_arg1),                                                 \
		  "0"(_num)                                                   \
		: "rcx", "r11", "memory", "cc"                                \
	);                                                                    \
	_ret;                                                                 \
})

static void *sys_brk(void *addr) {
	return (void *)syscall1(__NR_brk, addr);
}

static int brk(void *addr) {
	void *ret = sys_brk(addr);

	if (!ret) {
		return -1; // ENOMEM
	}
	return 0;
}

static void *sbrk(intptr_t inc) {
	void *ret;

	if ((ret = sys_brk(0)) && (sys_brk(ret + inc) == ret + inc))
		return ret + inc;

	return (void *)-1; // ENOMEM
}

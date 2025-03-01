//========= Copyright N11 Software, All rights reserved. ============//
//
// File: Syscall.cpp
// Purpose: Syscall Handler
// Maintainer: FiReLScar
//
//===================================================================//

#include <Drivers/TTY/COM.h>
#include <Interrupts/Syscall.hpp>
#include <Memory/Heap.hpp>
#include <Inferno/Log.h>

static uint64_t sys_malloc(uint64_t size, uint64_t unused) {
	void* ptr = Heap::Allocate(size);
	prInfo("syscall", "malloc(%d) = 0x%x", size, (uint64_t)ptr);
	return (uint64_t)ptr;
}

static uint64_t sys_free(uint64_t addr, uint64_t unused) {
	if (addr >= 0x400000 && addr < 0x500000) {
		Heap::Free((void*)addr);
		prInfo("syscall", "free(0x%x)", addr);
		return 0;
	}
	prErr("syscall", "Invalid free address: 0x%x", addr);
	return -1;
}

static syscall_handler_t syscall_table[] = {
	nullptr,	// 0
	nullptr,	// SYS_WRITE 
	sys_malloc, // SYS_MALLOC
	sys_free,   // SYS_FREE
};

__attribute__((interrupt)) void SyscallHandler(void*) {
	void* ptr = nullptr;
	
	asm volatile(
		"pushq %%rax\n"
		"pushq %%rdi\n"
		"pushfq"
		::: "memory");

	if (*(uint64_t*)(((uint8_t*)&ptr) + 16) == SYS_MALLOC) {
		ptr = Heap::Allocate(*(uint64_t*)(((uint8_t*)&ptr) + 8));
		asm volatile(
			"popfq\n"
			"popq %%rdi\n"
			"movq %0, %%rax\n"
			"leave\n"
			"iretq"
			:: "r"(ptr) : "memory");
	}
}

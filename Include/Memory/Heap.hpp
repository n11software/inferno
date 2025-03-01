#pragma once
#include <Inferno/stdint.h>
#include <Inferno/stddef.h>

// C memory functions must be in global scope
extern "C" {
	void* malloc(size_t size);
	void free(void* ptr);
	void* calloc(size_t num, size_t size);
	void* realloc(void* ptr, size_t size);
}

namespace Heap {
	struct HeapBlock {
		uint64_t size;
		bool used;
		HeapBlock* next;
	};

	void Initialize(uint64_t heapStart, uint64_t heapSize);
	void* Allocate(uint64_t size);
	void Free(void* ptr);
}

#pragma once
#include <Inferno/stdint.h>

namespace Memory {
	void* RequestPage();  // Returns a pointer to a new physical page
	void FreePage(void* address);
	void Initialize(void* bitmap, uint64_t size);
}

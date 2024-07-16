#include <Memory/Memory.hpp>
#include <Inferno/stddef.h>
#include <Memory/Paging.h>
#include <Memory/Mem_.hpp>
#include <Inferno/Log.h>
#include <Drivers/RTC/RTC.h>

unsigned long long Size = 0, Free, Used;
BOB* bob;

void Memory::Init(BOB* bob) {
    void* bitmapAddress = (void*)0;
    size_t bitmapSize = 0;
    for (unsigned long long i=0;i<bob->MapSize/bob->DescriptorSize; i++) {
        MemoryDescriptor* descriptor = (MemoryDescriptor*)((unsigned long long)bob->MemoryMap + (i * bob->DescriptorSize));
        Size+=descriptor->NumberOfPages*4096;
    }
	Free = Size;
}

unsigned long long Memory::GetSize() { return Size; }
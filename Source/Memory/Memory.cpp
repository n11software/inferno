#include <Memory/Memory.hpp>
#include <Inferno/stddef.h>
#include <Memory/Paging.h>
#include <Memory/Mem_.hpp>
#include <Inferno/Log.h>
#include <Drivers/RTC/RTC.h>

unsigned long long Size = 0;
BOB* bob;
unsigned long long free = 0, used = 0, locked = 0;

void Memory::Init(BOB* bob) {
    void* bitmapAddress = (void*)0;
    size_t bitmapSize = 0;
    for (unsigned long long i=0;i<bob->MapSize/bob->DescriptorSize; i++) {
        MemoryDescriptor* descriptor = (MemoryDescriptor*)((unsigned long long)bob->MemoryMap + (i * bob->DescriptorSize));
        Size+=descriptor->NumberOfPages*4096;
        if (descriptor->Type == 7 && descriptor->NumberOfPages*4096 > bitmapSize) {
            bitmapAddress = (void*)descriptor->PhysicalStart;
            bitmapSize = descriptor->NumberOfPages*4096;
        }
    }

    free = Size;
}

unsigned long long Memory::GetSize() { return Size; }
unsigned long long Memory::GetFree() { return free; }
unsigned long long Memory::GetUsed() { return used; }
unsigned long long Memory::GetLocked() { return locked; }

void Memory::SetFree(unsigned long long freeMem) { free = freeMem; }
void Memory::SetUsed(unsigned long long usedMem) { used = usedMem; }
void Memory::SetLocked(unsigned long long lockedMem) { locked = lockedMem; }
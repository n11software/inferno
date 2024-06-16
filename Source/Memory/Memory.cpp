#include <Memory/Memory.hpp>
#include <Inferno/stddef.h>
#include <Memory/Paging.h>
#include <Memory/Mem_.hpp>
#include <Inferno/Log.h>
#include <Drivers/RTC/RTC.h>

unsigned long long Size = 0;
BOB* bob;
unsigned long long free = 0, used = 0;

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

    kprintf("BM Address: 0x%x-0x%x\n", bitmapAddress, (unsigned long long)bitmapAddress+bitmapSize);
    kprintf("FB Address: 0x%x-0x%x\n", bob->framebuffer->Address, (unsigned long long)bob->framebuffer->Address+bob->framebuffer->Size);

    // Enable 4K allocation aka paging
    memset(bitmapAddress, 0, bitmapSize);
    Paging::map.Buffer = (unsigned char*)bitmapAddress;
    Paging::map.Size = Size/4096+1;

    Paging::AllocatePages(Paging::map.Buffer, Paging::map.Size/4096+1);
    Paging::AllocatePages(0, Size/4096+1);

    for (unsigned long long i=0;i<bob->MapSize/bob->DescriptorSize; i++) {
        MemoryDescriptor* descriptor = (MemoryDescriptor*)((unsigned long long)bob->MemoryMap + (i * bob->DescriptorSize));
        if (descriptor->Type == 7)
            Paging::FreePages(descriptor->PhysicalStart, descriptor->NumberOfPages);
    }
}

unsigned long long Memory::GetSize() { return Size; }
unsigned long long Memory::GetFree() { return free; }
unsigned long long Memory::GetUsed() { return used; }

void Memory::SetFree(unsigned long long freeMem) { free = freeMem; }
void Memory::SetUsed(unsigned long long usedMem) { used = usedMem; }
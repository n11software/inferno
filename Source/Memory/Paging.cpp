#include <Memory/Paging.h>
#include <Memory/Memory.hpp>

// TODO: Use memset and memcpy for bitmap and then use memset for multiple pages


Bitmap Paging::map;

void Paging::FreePage(void* address) {
  // Check if page is already free
  if (Paging::map[(unsigned long long)address/4096]==false) return;
  Paging::map.Set((unsigned long long)address/4096, false);
  Memory::SetFree(Memory::GetFree()+4096);
  Memory::SetUsed(Memory::GetUsed()-4096);
}

void Paging::AllocatePage(void* address) {
  // Check if page is already allocated
  if (Paging::map[(unsigned long long)address/4096]==true) return;
  if (!Paging::map.Set((unsigned long long)address/4096, true)) kprintf("Error whilst allocating 0x%x\n", address);
  Memory::SetFree(Memory::GetFree()-4096);
  Memory::SetUsed(Memory::GetUsed()+4096);
}

void Paging::FreePages(void* address, unsigned long long size) {
  for (unsigned long long x = 0; x < size; x++)
    Paging::FreePage((void*)((unsigned long long)address + (x*4096)));
}

void Paging::AllocatePages(void* address, unsigned long long size) {
  for (unsigned long long x = 0; x < size; x++)
    Paging::AllocatePage((void*)((unsigned long long)address + (x*4096)));
}

unsigned long long ReqIndex = 0;
void* Paging::RequestPage() {
  for (;ReqIndex<Paging::map.Size*8;ReqIndex++) {
    if (Paging::map[ReqIndex]==true) continue;
    Paging::AllocatePage((void*)(ReqIndex*4096));
    return (void*)(ReqIndex*4096);
    // TODO: Implement Look ahead
  }

  kprintf("Ran out of memory\n");
  // TODO: Swap File
  return (void*)0;
}

Paging::MapIndexer::MapIndexer(unsigned long long address) {
  address >>= 12;
  Level1Index = address&0x1ff;
  address >>= 9;
  Level2Index = address&0x1ff;
  address >>= 9;
  Level3Index = address&0x1ff;
  address >>= 9;
  Level4Index = address&0x1ff;
}

unsigned long long Paging::MapIndexer::operator[](unsigned char key) const{
  switch (key) {
    case 0: return Level1Index;
    case 1: return Level2Index;
    case 2: return Level3Index;
    case 3: return Level4Index;
    default: return 0;
  }
}

Paging::TableManager::TableManager(PageTable* Level4Address) {
  this->Level4Address = Level4Address;
}

void Paging::TableManager::Map(void* virt, void* address) {
  Paging::MapIndexer i((unsigned long long)virt);
  Page PDE;

  PDE = Level4Address->pages[i[3]];
  PageTable* PDP;
  if (!PDE.isPresent) {
    PDP = (PageTable*)Paging::RequestPage();
    memset(PDP, 0, 4096);

    PDE.Address = (unsigned long long)PDP>>12;
    PDE.isPresent = true;
    PDE.RW = true;
    Level4Address->pages[i[3]]=PDE;
  } else PDP = (PageTable*)((unsigned long long)PDE.Address<<12);

  PDE = PDP->pages[i[2]];
  PageTable* PD;
  if (!PDE.isPresent) {
    PD = (PageTable*)Paging::RequestPage();
    memset(PD, 0, 4096);

    PDE.Address = (unsigned long long)PD>>12;
    PDE.isPresent = true;
    PDE.RW = true;
    PD->pages[i[2]]=PDE;
  } else PD = (PageTable*)((unsigned long long)PDE.Address<<12);


  PDE = PD->pages[i[1]];
  PageTable* PT;
  if (!PDE.isPresent) {
    PT = (PageTable*)Paging::RequestPage();
    memset(PT, 0, 4096);

    PDE.Address = (unsigned long long)PT>>12;
    PDE.isPresent = true;
    PDE.RW = true;
    PD->pages[i[1]]=PDE;
  } else PT = (PageTable*)((unsigned long long)PDE.Address<<12);

  PDE = PT->pages[i[0]];
  PDE.Address = (unsigned long long)address>>12;
  PDE.isPresent = true;
  PDE.RW = true;
  PT->pages[i[0]]=PDE;
}
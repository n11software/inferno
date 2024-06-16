#pragma once
#include <Memory/Memory.hpp>

namespace Paging {
  extern Bitmap map;
  void FreePage(void* address);
  void AllocatePage(void* address);
  void FreePages(void* address, unsigned long long size);
  void AllocatePages(void* address, unsigned long long size);
  void* RequestPage();

  struct Page {
    bool isPresent:1;
    bool RW:1;
    bool Super:1;
    bool WriteThrough:1;
    bool isCached:1;
    bool isAccessed:1;
    bool null0:1;
    bool isLarger:1;
    bool null1:1;

    unsigned char KernelSpecific:3;
    unsigned long long Address:52;
  };

  struct PageTable {
    Page pages[512];
  }__attribute__((aligned(0x1000)));

  class MapIndexer {
    public:
      MapIndexer(unsigned long long address);
      unsigned long long operator[](unsigned char key) const;
    private:
      unsigned long long Level4Index, Level3Index, Level2Index, Level1Index;
  };

  class TableManager {
    public:
      TableManager(PageTable* Level4Address);
      void Map(void* virt, void* address);
    private:
      PageTable* Level4Address;
  };
}
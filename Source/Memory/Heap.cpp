#include <Memory/Heap.hpp>
#include <Memory/Memory.hpp>
#include <Memory/Mem_.hpp>
#include <Inferno/Log.h>

// Global new/delete operators
void* operator new(size_t size) {
    return Heap::Allocate(size);
}

void* operator new[](size_t size) {
    return Heap::Allocate(size);
}

void operator delete(void* ptr) noexcept {
    Heap::Free(ptr);
}

void operator delete[](void* ptr) noexcept {
    Heap::Free(ptr);
}

void operator delete(void* ptr, size_t size) noexcept {
    Heap::Free(ptr);
}

void operator delete[](void* ptr, size_t size) noexcept {
    Heap::Free(ptr);
}

// C memory functions
extern "C" {
    void* malloc(size_t size) {
        return Heap::Allocate(size);
    }

    void free(void* ptr) {
        Heap::Free(ptr);
    }

    void* calloc(size_t num, size_t size) {
        size_t total = num * size;
        void* ptr = Heap::Allocate(total);
        if (ptr) {
            memset(ptr, 0, total);
        }
        return ptr;
    }

    void* realloc(void* ptr, size_t size) {
        if (!ptr) return malloc(size);
        if (size == 0) {
            free(ptr);
            return nullptr;
        }

        void* new_ptr = malloc(size);
        if (!new_ptr) return nullptr;

        // Use proper namespace for HeapBlock
        Heap::HeapBlock* old_block = (Heap::HeapBlock*)((uint64_t)ptr - sizeof(Heap::HeapBlock));
        size_t copy_size = old_block->size < size ? old_block->size : size;
        memcpy(new_ptr, ptr, copy_size);
        free(ptr);

        return new_ptr;
    }
}

namespace Heap {
    static HeapBlock* firstBlock = nullptr;
    static uint64_t heapStart = 0;
    static uint64_t heapSize = 0;

    void Initialize(uint64_t start, uint64_t size) {
        heapStart = start;
        heapSize = size;
        
        // Create initial free block
        firstBlock = (HeapBlock*)start;
        firstBlock->size = size - sizeof(HeapBlock);
        firstBlock->used = false;
        firstBlock->next = nullptr;

        prInfo("heap", "Initialized at 0x%x, size: 0x%x", start, size);
    }

    void* Allocate(uint64_t size) {
        HeapBlock* current = firstBlock;
        HeapBlock* best = nullptr;
        uint64_t smallest = 0xFFFFFFFFFFFFFFFF;

        // Find best fit block
        while(current) {
            if (!current->used && current->size >= size) {
                if (current->size < smallest) {
                    best = current;
                    smallest = current->size;
                }
            }
            current = current->next;
        }

        if (!best) {
            prErr("heap", "allocation failed: no blocks available");
            return nullptr;
        }

        // Split block if it's too big
        if (best->size > size + sizeof(HeapBlock) + 16) {
            HeapBlock* newBlock = (HeapBlock*)((uint64_t)best + sizeof(HeapBlock) + size);
            newBlock->size = best->size - size - sizeof(HeapBlock);
            newBlock->used = false;
            newBlock->next = best->next;
            
            best->size = size;
            best->next = newBlock;
        }

        best->used = true;
        return (void*)((uint64_t)best + sizeof(HeapBlock));
    }

    void Free(void* ptr) {
        if (!ptr) return;

        HeapBlock* block = (HeapBlock*)((uint64_t)ptr - sizeof(HeapBlock));
        block->used = false;

        // Merge with next block if free
        while (block->next && !block->next->used) {
            block->size += sizeof(HeapBlock) + block->next->size;
            block->next = block->next->next;
        }
    }
}

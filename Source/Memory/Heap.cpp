#include <Inferno/stddef.h>
#include <Memory/Paging.h>
#include <Memory/Heap.h>
#include <Drivers/TTY/COM.h>

extern Paging* paging;
extern PhysicalMemoryManager pmm;
extern unsigned long long Free, Used, Size;

void* HeapAddress;
unsigned long long HeapSize;

void* HeapEnd;
SegmentHeader* LastHeader;

void InitializeHeap(void* address, size_t size) {
	kprintf("Heap initializing\n");
	void* pos = address;
	for (size_t i=0; i<size; i++) {
		unsigned long long newAddr = (unsigned long long)pmm.alloc_page();
		paging->map_page(newAddr, (unsigned long long)pos);
		pos = (void*)((size_t)pos + 4096);
	}
	size_t len = size*4096;
	HeapAddress = address;
	HeapEnd = (void*)((size_t)HeapAddress+len);
	SegmentHeader* st = (SegmentHeader*)address;
	st->length = len-sizeof(SegmentHeader);
	st->next = NULL;
	st->last = NULL;
	st->isFree = true;
	LastHeader = st;
}

void* malloc(size_t size) {
	if (size % 0x10 > 0) {
		size -= (size%0x10);
		size += 0x10;
	}
	if (size == 0) return NULL;
	SegmentHeader* current = (SegmentHeader*)HeapAddress;
	while (1) {
		if (current->isFree) {
			if (current->length > size) {
				current->Split(size);
				current->isFree = false;
				return (void*)((uint64_t)current + sizeof(SegmentHeader));
			}
			if (current->length == size) {
				current->isFree = false;
				return (void*)((uint64_t)current + sizeof(SegmentHeader));
			}
		}
		if (current->next == NULL) break;
		current = current->next;
	}
	ExpandHeap(size);
	return malloc(size);
}

void free(void* address) {
    SegmentHeader* segment = (SegmentHeader*)address - 1;
    segment->isFree = true;
    segment->CombineForward();
    segment->CombineBackward();
}

void ExpandHeap(size_t size) {
    if (size % 0x1000) {
        size -= size % 0x1000;
        size += 0x1000;
    }

    size_t pageCount = size / 0x1000;
    SegmentHeader* newSegment = (SegmentHeader*)HeapEnd;

    for (size_t i = 0; i < pageCount; i++){
        paging->map_page((unsigned long long)pmm.alloc_page(), (unsigned long long)HeapEnd);
        HeapEnd = (void*)((size_t)HeapEnd + 0x1000);
    }

    newSegment->isFree = true;
    newSegment->last = LastHeader;
    LastHeader->next = newSegment;
    LastHeader = newSegment;
    newSegment->next = NULL;
    newSegment->length = size - sizeof(SegmentHeader);
    newSegment->CombineBackward();
}

SegmentHeader* SegmentHeader::Split(size_t splitLength){
    if (splitLength < 0x10) return NULL;
    int64_t splitSegLength = length - splitLength - (sizeof(SegmentHeader));
    if (splitSegLength < 0x10) return NULL;

    SegmentHeader* newSplitHdr = (SegmentHeader*) ((size_t)this + splitLength + sizeof(SegmentHeader));
    next->last = newSplitHdr; // Set the next segment's last segment to our new segment
    newSplitHdr->next = next; // Set the new segment's next segment to out original next segment
    next = newSplitHdr; // Set our new segment to the new segment
    newSplitHdr->last = this; // Set our new segment's last segment to the current segment
    newSplitHdr->length = splitSegLength; // Set the new header's length to the calculated value
    newSplitHdr->isFree = free; // make sure the new segment's free is the same as the original
    length = splitLength; // set the length of the original segment to its new length

    if (LastHeader == this) LastHeader = newSplitHdr;
    return newSplitHdr;
}

void SegmentHeader::CombineForward(){
    if (next == NULL) return;
    if (!next->isFree) return;

    if (next == LastHeader) LastHeader = this;

    if (next->next != NULL){
        next->next->last = this;
    }

    length = length + next->length + sizeof(SegmentHeader);
}

void SegmentHeader::CombineBackward(){
    if (last != NULL && last->isFree) last->CombineForward();
}
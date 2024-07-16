#pragma once
#include <Inferno/stddef.h>

struct SegmentHeader {
	size_t length;
	SegmentHeader* next, *last;
	bool isFree;
	void CombineForward();
	void CombineBackward();
	SegmentHeader* Split(size_t length);
};

void InitializeHeap(void* address, size_t size);
void* malloc(size_t);
void free(void*);
void ExpandHeap(size_t size);
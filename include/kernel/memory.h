
#pragma once

#include "kernel.h"

struct Heap_Header;

void heap_init();
void* get_found_block(struct Heap_Header*, size);
void* heap_allocate(size);
void heap_deallocate(void*);

#define heap_allocate_typed(Type, count) \
    (Type*) heap_allocate((count) * size_of(Type));


#pragma once

#include "common.h"

struct Heap_Header;

void heap_init();
void* heap_allocate(size);
void heap_deallocate(void*);

#define heap_allocate_typed(Type, count) \
    (Type*) heap_allocate((count) * size_of(Type));



void mem_copy(void*, void*, size);
void mem_move(void*, void*, size);
void mem_set(void*, size, u8);
void mem_clear(void*, size);
i32  mem_compare(void*, void*, size);

#define mem_set_typed(Type, target, count, value)     \
    for (size n = 0; n < count; n++) {                \
        cast(Type*, target)[n] = value;               \
    }                                                 \



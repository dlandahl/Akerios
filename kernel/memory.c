
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "drivers/vga.h"

struct Heap_Header {
    struct Heap_Header* next_header;
    bool free;
};

struct Heap_Header* heap_first_header = (struct Heap_Header*) 0x40000;

void heap_init() {
    heap_first_header->next_header = nullptr;
    heap_first_header->free = true;
}

internal void* get_found_block(struct Heap_Header* header, size bytes) {
    struct Heap_Header* new_header
        = cast(void*, header) + bytes + size_of(struct Heap_Header);

    new_header->next_header = header->next_header;
    header->next_header     = new_header;
    new_header->free = true;
    header->free     = false;
    return cast(void*, header) + size_of(struct Heap_Header);
}

void* heap_allocate(size bytes) {
    struct Heap_Header* current = heap_first_header;
    while (true) {
        while (!current->free) {
            current = current->next_header;
        }

        if (!current->next_header) {
            return get_found_block(current, bytes);
        }

        const size block_size = cast(void*, current->next_header)
                              - cast(void*, current);
        if (block_size - size_of(struct Heap_Header) >= bytes) {
            return get_found_block(current, bytes);
        }
        else current = current->next_header;
    }
}

void heap_deallocate(void* block) {
    struct Heap_Header* header = cast(void*, block) - size_of(struct Heap_Header);
    header->free = true;
    while (header->next_header && header->next_header->free) {
        header->next_header = header->next_header->next_header;
    }
}



void mem_copy(void* dest, void* src, size bytes) {
    for (size n = 0; n < bytes; n++) {
        cast(u8*, dest)[n] = cast(u8*, src)[n];
    }
}

void mem_move(void* dest, void* src, size bytes) {
    if (dest > src && dest - src < bytes
     || dest < src && src - dest < bytes) {
        void* temp = heap_allocate(bytes);
        mem_copy(temp, src, bytes);
        mem_copy(dest, temp, bytes);
        heap_deallocate(temp);
    }
    else mem_copy(dest, src, bytes);
}

void mem_set(void* target, size bytes, u8 value) {
    for (size n = 0; n < bytes; n++) {
        cast(u8*, target)[n] = value;
    }
}

void mem_clear(void* target, size bytes) {
    mem_set(target, bytes, 0);
}

i32 mem_compare(void* a, void* b, size bytes) {
    for (size n = 0; n < bytes; n++) {
        const int diff = cast(u8*, a)[n] - cast(u8*, b)[n];
        if (diff == 0) continue;
        if (diff >  0) return  1;
        return -1;
    }
}

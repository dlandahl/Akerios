
#include "kernel.h"
#include "memory.h"

struct Heap_Header* heap_first_header = (struct Heap_Header*) 0x4000;

void heap_init() {
    heap_first_header->next_header = nullptr;
    heap_first_header->free = true;
}

void* get_found_block(struct Heap_Header* header, size bytes) {
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

        if_not (current->next_header) {
            return get_found_block(current, bytes);
        }

        const size block_size = cast(void*, current->next_header)
                              - cast(void*, current);
        if (block_size - size_of(struct Heap_Header) > bytes) {
            return get_found_block(current, bytes);
        }
    }
}

void heap_deallocate(void* block) {

}

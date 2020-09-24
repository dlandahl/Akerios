
#include "common.h"
#include "kernel/memory.h"
#include "kernel/kernel.h"
#include "kernel/filesystem.h"
#include "drivers/ata.h"
#include "drivers/vga.h"

enum {
    fat_fat =          (Fat_Entry) -2,
    fat_end_of_chain = (Fat_Entry) -1,
    fat_unused = 0,
};

#define fat_entry_count (fs_blocksize / size_of(Fat_Entry))
struct Fat_Block {
    Fat_Entry entries[fat_entry_count];
} __attribute__((packed));



struct Dir_Entry {
    i8 name[64];
    bool is_dir;
    size size;
    Fat_Entry index;
};

#define dir_entry_count (fs_blocksize / size_of(struct Dir_Entry))
struct Dir_Block {
    struct Dir_Entry entries[dir_entry_count];
    u8 _[fs_blocksize - size_of(struct Dir_Entry[dir_entry_count])];
} __attribute__((packed));



struct Data_Block {
    u8 data[fs_blocksize];
} __attribute__((packed));

union Block {
    struct Fat_Block  fat_block;
    struct Dir_Block  dir_block;
    struct Data_Block data_block;
} __attribute__((packed));



// For some reason I can't get the ATA driver to read or write several sectors at once?
internal Ata_Error read_block_from_disk(union Block* block, size index) {
    const size sectors_per_block = fs_blocksize / 512;
    for (size sector = 0; sector < sectors_per_block; sector++) {
        ata_lba_read(cast(void*, block) + sector * 512, fs_disk_location + sector + index * sectors_per_block, 1);
    }
    return ata_get_error();
}

internal Ata_Error write_block_to_disk(union Block* block, size index) {
    const size sectors_per_block = fs_blocksize / 512;
    for (size sector = 0; sector < sectors_per_block; sector++) {
        ata_lba_write(cast(void*, block) + sector * 512, fs_disk_location + sector + index * sectors_per_block, 1);
    }
    return ata_get_error();
}

internal Fat_Entry get_free_fat_entry() {
    // FAT should be cached in main memory
    union Block fat;
    read_block_from_disk(&fat, 0);
    for (Fat_Entry n = 0; n < fat_entry_count; n++) {
        if (fat.fat_block.entries[n] == fat_unused) return n;
    }
    return -1;
}

internal size find_file(u8* name, struct Dir_Block* dir) {
    size dir_entry = -1;
    for (size n = 0; n < dir_entry_count; n++) {
        if (str_compare(dir->entries[n].name, name)) {
            dir_entry = n;
            break;
        }
    }
    return dir_entry;
}



bool fs_write_entire_file(i8* name, u8* buffer, size file_size) {
    struct Dir_Block root;
    read_block_from_disk(cast(union Block*, &root), 1);

    size dir_entry = find_file(name, &root);
    Fat_Entry entry = root.entries[dir_entry].index;

    struct Fat_Block fat;
    read_block_from_disk(cast(union Block*, &fat), 0);

    size blocks_required = (file_size-1) / fs_blocksize;
    vga_print("\nExtra blocks required: ");
    vga_print_hex(blocks_required);
    vga_newline();

    do {
        write_block_to_disk(cast(union Block*, buffer), entry);
        buffer += fs_blocksize;

        if (blocks_required && (fat.entries[entry] == fat_end_of_chain)) {
            write_block_to_disk(cast(union Block*, &fat), 0);
            Fat_Entry new_block = get_free_fat_entry();
            fat.entries[entry] = new_block;
        }
        entry = fat.entries[entry];
        fat.entries[entry] = fat_end_of_chain;
    } while (blocks_required--);

    // Mark blocks that are no longer required as unused.
    // This happens when the data we write to the file is less than the original file size.
    // while (fat.entries[entry_to_clear] != fat_end_of_chain) {
    //     Fat_Entry next = fat.entries[entry_to_clear];
    //     fat.entries[entry_to_clear] = fat_unused;
    //     entry_to_clear = next;
    // }
    root.entries[dir_entry].size = file_size;
    mem_copy(root.entries[dir_entry].name, name, str_length(name));

    write_block_to_disk(cast(union Block*, &fat), 0);
    write_block_to_disk(cast(union Block*, &root), 1);
    return true;
}

u8* fs_read_entire_file(i8* name) {
    struct Dir_Block root;
    read_block_from_disk(cast(union Block*, &root), 1);

    struct Fat_Block fat;
    read_block_from_disk(cast(union Block*, &fat), 0);

    size dir_entry = find_file(name, &root);
    if (dir_entry == -1) return nullptr;
    Fat_Entry entry = root.entries[dir_entry].index;
    size file_size  = root.entries[dir_entry].size;

    u8* buffer = heap_allocate(file_size);
    while (true) {
        read_block_from_disk(cast(union Block*, buffer), entry);
        entry = fat.entries[entry];
        if (entry == fat_end_of_chain) break;
        buffer += fs_blocksize;
    }
    return buffer;
}

void fs_create_file(i8* name) {
    struct Fat_Block fat;
    read_block_from_disk(cast(union Block*, &fat), 0);

    struct Dir_Block root;
    read_block_from_disk(cast(union Block*, &root), 1);

    Fat_Entry index = get_free_fat_entry();
    fat.entries[index] = fat_end_of_chain;
    vga_print("Created empty file in: ");
    vga_print_hex(index);
    vga_newline();

    for (size dir_entry = 0; dir_entry < dir_entry_count; dir_entry++) {
        struct Dir_Entry* entry = &root.entries[dir_entry];
        if (entry->index == fat_unused) {
            mem_copy(entry->name, name, str_length(name));
            entry->size = 0;
            entry->index = index;
            break;
        }
    }

    write_block_to_disk(cast(union Block*, &fat), 0);
    write_block_to_disk(cast(union Block*, &root), 1);
}

Ata_Error fs_format() {
    struct Fat_Block fat;
    mem_clear(&fat,  fs_blocksize);

    fat.entries[0] = fat_fat;
    fat.entries[1] = fat_end_of_chain;

    Ata_Error err = 0;
    err |= write_block_to_disk(cast(union Block*, &fat), 0);

    struct Dir_Block root;
    mem_clear(&root, fs_blocksize);

    err |= write_block_to_disk(cast(union Block*, &root), 1);
    return err;
}

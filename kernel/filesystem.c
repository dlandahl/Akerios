
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

    // Padding to ensure the size is 1 disk block
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

// Cache
struct Fat_Block fat;
struct Dir_Block root;



// For some reason I can't get the ATA driver to read or write several sectors in one go
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
    for (Fat_Entry n = 0; n < fat_entry_count; n++) {
        if (fat.entries[n] == fat_unused) return n;
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

internal void write_data(Fat_Entry entry, void* buffer, size count) {
    size blocks_required = (count-1) / fs_blocksize;

    union Block block;
    do {
        mem_clear(&block, fs_blocksize);
        mem_copy(block.data_block.data, buffer, min(fs_blocksize, count));
        write_block_to_disk(&block, entry);
        buffer += fs_blocksize;

        if (blocks_required && (fat.entries[entry] == fat_end_of_chain)) {
            Fat_Entry new_block = get_free_fat_entry();
            fat.entries[entry] = new_block;
        }
        entry = fat.entries[entry];
        fat.entries[entry] = fat_end_of_chain;
        count -= fs_blocksize;
    } while (blocks_required--);
}

internal void clear_entry_list(Fat_Entry entry_to_clear) {
    while (entry_to_clear != fat_end_of_chain) {
        Fat_Entry next = fat.entries[entry_to_clear];
        fat.entries[entry_to_clear] = fat_unused;
        entry_to_clear = next;
    }
}



bool fs_write_entire_file(u8* name, void* buffer, size file_size) {
    size dir_entry = find_file(name, &root);
    if (dir_entry == -1) return nullptr;
    Fat_Entry entry = root.entries[dir_entry].index;

    clear_entry_list(fat.entries[entry]);
    write_data(entry, buffer, file_size);

    root.entries[dir_entry].size = file_size;
    mem_copy(root.entries[dir_entry].name, name, str_length(name));

    return true;
}

void* fs_read_entire_file(u8* name) {
    size dir_entry = find_file(name, &root);
    if (dir_entry == -1) return nullptr;
    Fat_Entry entry = root.entries[dir_entry].index;
    size file_size  = root.entries[dir_entry].size;

    u8* buffer = heap_allocate(file_size);
    u8* ret = buffer;
    while (true) {
        read_block_from_disk(cast(union Block*, buffer), entry);
        entry = fat.entries[entry];
        if (entry == fat_end_of_chain) break;
        buffer += fs_blocksize;
    }
    return ret;
}

bool fs_append_to_file(u8* name, void* buffer, i32 count) {
    size dir_entry = find_file(name, &root);
    if (dir_entry == -1) return false;
    struct Dir_Entry* file = &root.entries[dir_entry];

    Fat_Entry entry = file->index;
    while (fat.entries[entry] != fat_end_of_chain) {
        entry = fat.entries[entry];
    }

    i32 residue = fs_blocksize - (file->size % fs_blocksize);
    if (residue) {
        i32 bytes_to_write = (count < residue) ? count : residue;
        void* data = heap_allocate(bytes_to_write);
        read_block_from_disk(cast(union Block*, data), entry);
        mem_copy(data + fs_blocksize - residue, buffer, bytes_to_write);
        write_block_to_disk(cast(union Block*, data), entry);

        buffer += bytes_to_write;
        count -= bytes_to_write;
        file->size += bytes_to_write;
    }
    if (count <= 0) return true;

    // write_data(entry, buffer, count);
    // file->size += count;
}

void fs_create_file(u8* name) {
    Fat_Entry index = get_free_fat_entry();
    fat.entries[index] = fat_end_of_chain;

    for (size dir_entry = 0; dir_entry < dir_entry_count; dir_entry++) {
        struct Dir_Entry* entry = &root.entries[dir_entry];
        if (entry->index == fat_unused) {
            mem_copy(entry->name, name, str_length(name));
            entry->size = 0;
            entry->index = index;
            break;
        }
    }
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

    fs_init();
    return err;
}

void fs_init() {
    read_block_from_disk(cast(union Block*, &fat), 0);
    read_block_from_disk(cast(union Block*, &root), 1);
}

void fs_commit() {
    write_block_to_disk(cast(union Block*, &fat), 0);
    write_block_to_disk(cast(union Block*, &root), 1);
}

void fs_list_directory() {
    for (size n = 0; n < dir_entry_count; n++) {
        struct Dir_Entry* entry = &root.entries[n];
        if_not (entry->index) continue;
        vga_print(entry->name);
        vga_print(":\tFile size: ");
        vga_print_hex(entry->size);
        vga_print("  |  FAT entry: ");
        vga_print_hex(entry->index);
        vga_newline();
    }
}

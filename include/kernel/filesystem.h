
#pragma once

#include "common.h"

#define fs_blocksize 2048
#define fs_disk_location ((1 << 16) / 512)

typedef u16 Fat_Entry;

bool fs_write_entire_file(u8*, void*, size);
void* fs_read_entire_file(u8*);
bool fs_append_to_file(u8*, void*, i32);
void fs_create_file(u8*);
u8 fs_format();
void fs_init();
void fs_commit();
void fs_list_directory();

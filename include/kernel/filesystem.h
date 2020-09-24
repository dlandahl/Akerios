
#pragma once

#include "common.h"

#define fs_blocksize 2048
#define fs_disk_location ((1 << 16) / 512)

typedef u16 Fat_Entry;

bool fs_write_entire_file(i8*, u8*, size);
u8* fs_read_entire_file(i8*);
void fs_create_file(i8*);
u8 fs_format();


#pragma once

#include "common.h"

enum {
    ata_status_error               = 1 << 0,
    ata_status_index               = 1 << 1,
    ata_status_corrected_data      = 1 << 2,
    ata_status_transfer_requested  = 1 << 3,
    ata_status_seek_complete       = 1 << 4,
    ata_status_device_fault        = 1 << 5,
    ata_status_ready               = 1 << 6,
    ata_status_busy                = 1 << 7,
};

enum {
    ata_err_address_mark_not_found = 1 << 0,
    ata_err_track_zero_not_found   = 1 << 1,
    ata_err_aborted                = 1 << 2,
    ata_err_media_change_request   = 1 << 3,
    ata_err_id_not_found           = 1 << 4,
    ata_err_media_changed          = 1 << 5,
    ata_err_data_error             = 1 << 6,
    ata_err_bad_block              = 1 << 7,
};

typedef u8 Ata_Error;
typedef u8 Ata_Status;

struct Ata_Drive {
    u32 spt, hpc;
};

struct Ata_Chs_Address {
    u32 c, h, s;
};

size ata_chs_to_lba(struct Ata_Drive, struct Ata_Chs_Address);
Ata_Error ata_lba_read(void*, size, u8);
Ata_Error ata_lba_write(void*, size, u8);

Ata_Error  ata_get_error();
Ata_Status ata_get_status();

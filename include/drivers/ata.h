
#pragma once

enum Ata_Status {
    ata_err   = 1 << 0,
    ata_idx   = 1 << 1,
    ata_corr  = 1 << 2,
    ata_drq   = 1 << 3,
    ata_srv   = 1 << 4,
    ata_df    = 1 << 5,
    ata_rdy   = 1 << 6,
    ata_bsy   = 1 << 7,
};

enum Ata_Error {
    ata_amnf  = 1 << 0,
    ata_tkznf = 1 << 1,
    ata_abrt  = 1 << 2,
    ata_mcr   = 1 << 3,
    ata_idnf  = 1 << 4,
    ata_mc    = 1 << 5,
    ata_unc   = 1 << 6,
    ata_bbk   = 1 << 7,
};

struct Ata_Drive {
    int spt, hpc;
};

struct Ata_Chs_Address {
    int c, h, s;
};

int ata_chs_to_lba(struct Ata_Drive, struct Ata_Chs_Address);
enum Ata_Status ata_lba_read(size, u8, void*);

enum Ata_Error  ata_get_error();
enum Ata_Status ata_get_status();


#pragma once

struct Ata_Drive {
    int spt, hpc;
};

struct Ata_Chs_Address {
    int c, h, s;
};

int  ata_chs_to_lba(struct Ata_Drive, struct Ata_Chs_Address);
void ata_lba_read(size, u8, void*);

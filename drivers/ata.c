
#include "common.h"
#include "drivers/ata.h"
#include "kernel/kernel.h"

enum Ata_Port_Out {
     ata_port_data          = 0x1f0,
     ata_port_feature       = 0x1f1,
     ata_port_sector_count  = 0x1f2,
     ata_port_sector_number = 0x1f3,
     ata_port_cylinder_low  = 0x1f4,
     ata_port_cylinder_high = 0x1f5,
     ata_port_drive_header  = 0x1f6,
     ata_port_command       = 0x1f7,
};

enum Ata_Port_In {
    ata_port_error  = 0x1f1,
    ata_port_status = 0x1f7,
};

enum Ata_Cmd {
    ata_cmd_read    = 0x20,
    ata_cmd_write   = 0x30,
};

enum Ata_Status {
    ata_err  = 1 << 0,
    ata_idx  = 1 << 1,
    ata_corr = 1 << 2,
    ata_drq  = 1 << 3,
    ata_srv  = 1 << 4,
    ata_df   = 1 << 5,
    ata_rdy  = 1 << 6,
    ata_bsy  = 1 << 7,
};

int ata_chs_to_lba(struct Ata_Drive drive, struct Ata_Chs_Address addr) {
    return (addr.c * drive.hpc + addr.h) * drive.spt + (addr.s - 1);
}

void ata_lba_read(size logical_address, u8 sectors_to_read, void* buffer) {
    const u8 lba_mode = 0b11100000;
    port_write(ata_port_drive_header,  cast(u8, logical_address >> 24) | lba_mode);
    port_write(ata_port_sector_count,  sectors_to_read);

    port_write(ata_port_sector_number, cast(u8, logical_address));
    port_write(ata_port_cylinder_low,  cast(u8, logical_address >> 8));
    port_write(ata_port_cylinder_high, cast(u8, logical_address >> 16));

    port_write(ata_port_command, ata_cmd_read);

    while (port_read(ata_port_status) & ata_drq);

    const size words_per_sector = 256;
    const size words_to_read = sectors_to_read * words_per_sector;
    asm("rep insw" :: "c"(words_to_read),
                      "d"(ata_port_data),
                      "D"(buffer));
}

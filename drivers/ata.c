
#include "common.h"
#include "drivers/ata.h"
#include "kernel/kernel.h"

enum Ata_Port_Out {
     ata_port_data          = 0x1f0,
     ata_port_feature       = 0x1f1,
     ata_port_sector_count  = 0x1f2,
     ata_port_lba_low       = 0x1f3,
     ata_port_lba_mid       = 0x1f4,
     ata_port_lba_high      = 0x1f5,
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

size ata_chs_to_lba(struct Ata_Drive drive, struct Ata_Chs_Address addr) {
    return (addr.c * drive.hpc + addr.h) * drive.spt + (addr.s - 1);
}

internal void ata_await_not_busy() {
    while (ata_get_status() & ata_status_busy);
}

internal void ata_await_ready() {
    while (!(ata_get_status() & ata_status_ready)) io_wait();
}

internal void ata_send_address_and_size(size logical_address, u8 sectors_to_read) {
    const u8 lba_mode = 0b11100000;
    port_write(ata_port_drive_header,  cast(u8, logical_address >> 24) | lba_mode);
    port_write(ata_port_sector_count,  sectors_to_read);

    port_write(ata_port_lba_low,  cast(u8, logical_address));
    port_write(ata_port_lba_mid,  cast(u8, logical_address >> 8));
    port_write(ata_port_lba_high, cast(u8, logical_address >> 16));
    ata_await_not_busy();
    ata_await_ready();
}

Ata_Error ata_lba_read(void* buffer, size logical_address, u8 sectors_to_read) {
    ata_send_address_and_size(logical_address, sectors_to_read);

    port_write(ata_port_command, ata_cmd_read);
    ata_await_not_busy();

    const size words_per_sector = 256;
    const size words_to_read = sectors_to_read * words_per_sector;
    asm("rep insw" :: "c"(words_to_read),
                      "d"(ata_port_data),
                      "D"(buffer));

    io_wait();
    return ata_get_error();
}

Ata_Error ata_lba_write(void* buffer, size logical_address, u8 sectors_to_write) {
    ata_send_address_and_size(logical_address, sectors_to_write);
    port_write(ata_port_command, ata_cmd_write);
    ata_await_not_busy();

    const size words_per_sector = 256;
    const size words_to_write = sectors_to_write * words_per_sector;
    asm("rep outsw" :: "c"(words_to_write),
                       "d"(ata_port_data),
                       "S"(buffer));

    ata_await_not_busy();
    return ata_get_error();
}

Ata_Error ata_get_error() {
    return port_read(ata_port_error);
}

Ata_Status ata_get_status() {
    return port_read(ata_port_status);
}

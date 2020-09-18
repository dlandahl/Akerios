
#include "kernel/kernel.h"
#include "drivers/vga.h"

enum {
    vga_reg_input_status      = 0x3da,
    vga_reg_attribute_address = 0x3c0,
    vga_reg_data_read         = 0x3c1,
};

void vga_set_attribute(u8 register_index, size bit, bool unset) {
    const u8 PAS  = 1 << 5;
    const u8 mask = 1 << bit;

    port_read(vga_reg_input_status);
    const u8 address = port_read(vga_reg_attribute_address);
    port_write(vga_reg_attribute_address, register_index | PAS);

    u8 data = port_read(vga_reg_data_read);
    data = unset ? (data & ~mask)
                 : (data |  mask);
    port_write(vga_reg_attribute_address, data);

    port_write(vga_reg_attribute_address, address);
}

void vga_hide_cursor() {
    port_write(0x3d4, 0x0a);
    port_write(0x3d5, 0x20);
}

void vga_move_cursor(u32 loc) {
    port_write(0x3d4, 0x0f);
    port_write(0x3d5, (u8) loc);
    port_write(0x3d4, 0x0e);
    port_write(0x3d5, (u8) (loc >> 8));
}

void vga_init() {
    vga.cursor = 0;
    vga.attribute = 0x83;
    vga.framebuffer = (i8*) 0xb8000;
    vga.framebuffer_size = vga_cols * vga_rows * sizeof(u16);

    vga_set_attribute(0x10, 3, true);
    vga.tab_stop = 4;
}

void vga_newline() {
    vga.cursor += vga_cols - vga.cursor % vga_cols;
}

void vga_tab() {
    size start = vga.cursor - (vga.cursor % vga_cols);
    size trail = (vga.cursor - start) % vga.tab_stop;
    vga.cursor += vga.tab_stop - trail;
}

void vga_clear() {
    for (size n = 0; n < vga.framebuffer_size / 2; n++) {
        vga.framebuffer[n*2]   = ' ';
        vga.framebuffer[n*2+1] = vga.attribute;
    }
    vga.cursor = 0;
}

void vga_print(i8* message) {
    for (size n = 0; message[n]; n++) {
        size index = vga.cursor * 2;

        switch (message[n]) {
            case '\n': vga_newline(); break;
            case '\t': vga_tab();     break;
            default:
                vga.framebuffer[index]   = message[n];
                vga.framebuffer[index+1] = vga.attribute;
                vga.cursor++;
        }
    }
}

void vga_put(i8* message, size col, size row) {
    size old = vga.cursor;
    vga.cursor = vga_cols * row + col;
    vga_print(message);
    vga.cursor = old;
}

void vga_print_char(i8 character) {
    static i8 buffer[2] = { 0 };
    buffer[0] = character;
    vga_print(buffer);
}

void vga_print_hex(u32 number) {
    u8 buffer[9] = { 0 };
    const static i8 hex_digits[] = "0123456789abcdef";
    for (size n = 0; n < 8; n++) {
        buffer[7-n] = hex_digits[number & 0xf];
        number >>= 4;
    }
    vga_print("0x");
    vga_print(buffer);
}

void vga_print_byte(u8 number) {
    vga_print("0b");
    u8 mask = 1 << 7;
    for (size n = 0; n < 8; n++) {
        vga_print_char((number & mask) ? '1' : '0');
        mask >>= 1;
    }
}

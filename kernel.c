
#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef int32_t  size;

typedef enum {
    false, true
} bool;

size strlen(u8* string) {
    size n;
    for (n = 0; string[n]; n++);
    return n;
}

void port_write_u8(u16 port, u8 value) {
    asm("out dx, al" :: "a"(value),  "d"(port));
}

u8 port_read_u8(u16 port) {
    u8 value;
    asm("in al, dx" : "=a"(value) : "d"(port));
    return value;
}

void port_write_u16(u16 port, u16 value) {
    asm("out dx, ax" :: "a"(value),  "d"(port));
}

u16 port_read_u16(u16 port) {
    u16 value;
    asm("in ax, dx" : "=a"(value) : "d"(port));
    return value;
}



#define vga_cols 80
#define vga_rows 35

struct {
    u8 attribute;
    u8* framebuffer;
    size cursor;
    size framebuffer_size;
} vga;

typedef enum {
    input_status      = 0x3da,
    attribute_address = 0x3c0,
    data_read         = 0x3c1,
} Vga_Register;

void vga_set_attribute(u8 register_index, size bit, bool unset) {
    const u8 PAS       = 1 << 5;

    port_read_u8(input_status);
    const u8 address = port_read_u8(attribute_address);

    port_write_u8(attribute_address, register_index | PAS);

    u8 data = port_read_u8(data_read);
    const u8 mask = 1 << bit;

    data = unset ? (data & ~mask)
                 : (data |  mask);

    port_write_u8(attribute_address, data);
    port_write_u8(attribute_address, address);
}

void vga_init() {
    vga.cursor = 0;
    vga.attribute = 0xf4;
    vga.framebuffer = (u8*) 0xb8000;
    vga.framebuffer_size = vga_cols * vga_rows * sizeof(u16);

    vga_set_attribute(0x10, 3, true);
}

void vga_newline() {
    vga.cursor += vga_cols - vga.cursor % vga_cols;
}

void vga_clear() {
    for (size n = 0; n < vga.framebuffer_size; n++) {
        vga.framebuffer[n] = 0;
    }
}

void vga_print(u8* message) {
    for (size n = 0; message[n]; n++) {
        size index = vga.cursor * 2;

        switch (message[n]) {
            case '\n': vga_newline();   break;
            case '\t': vga.cursor += 4; break;
            default:
                vga.framebuffer[index]   = message[n];
                vga.framebuffer[index+1] = vga.attribute;
                vga.cursor++;
        }
    }
}



u8* phrase = "Hello, everyone.";

void entry_point() {
    vga_init();
    vga_clear();
    vga_newline();
    vga_print(phrase);

    vga_print(" We like to have fun here.");
    vga.attribute = 0xe3;

    vga_print("\n\n\tWell well well.");
    while (true);
}


#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef int32_t  u32;
typedef u32      size;
typedef float    real;

typedef enum {
    false, true
} bool;

size strlen(u8* string) {
    size n;
    for (n = 0; string[n]; n++);
    return n;
}

void port_write_u8(u16 port, u8 value) {
    asm("out dx, al" :: "a"(value), "d"(port));
}

u8 port_read_u8(u16 port) {
    u8 value;
    asm("in al, dx" : "=a"(value) : "d"(port));
    return value;
}

__attribute__ ((alias ("port_read_u8" )))
u8   port_read  (u16);

__attribute__ ((alias ("port_write_u8")))
void port_write (u16, u8);

void port_write_u16(u16 port, u16 value) {
    asm("out dx, ax" :: "a"(value),  "d"(port));
}

u16 port_read_u16(u16 port) {
    u16 value;
    asm("in ax, dx" : "=a"(value) : "d"(port));
    return value;
}

typedef struct {
    u32 lower, upper;
} u64;

u64 rdtsc() {
    u64 tsc;
    asm("rdtsc" : "=d"(tsc.upper), "=a"(tsc.lower));
    return tsc;
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
    vga_reg_input_status      = 0x3da,
    vga_reg_attribute_address = 0x3c0,
    vga_reg_data_read         = 0x3c1,
} Vga_Register;

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



typedef struct {
    u32 state;
} Rand;

u32 rand_next_int(Rand* rand) {
    u32 x = rand->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rand->state = x;
    return x;
}

u32 rand_range(Rand* rand, u32 min, u32 max) {
    u32 value = rand_next_int(rand);
    value %= max - min;
    return value + min + 1;
}

void rand_set_seed(Rand* rand, u32 seed) {
    rand->state = seed;
}



#define pit_dividend 1193182.f

enum {
    pit_port_channel_0 = 0x40,
    pit_port_channel_1 = 0x41,
    pit_port_channel_2 = 0x42,
    pit_port_command   = 0x43,
};

u16 pit_set_frequency(real frequency) {
    u16 divisor = pit_dividend / frequency;

    port_write(pit_port_command, 0x36);
    port_write(pit_port_channel_0, (u8) (divisor      & 0xff));
    port_write(pit_port_channel_0, (u8) (divisor >> 8 & 0xff));

    return divisor;
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

    /*
        u64 tsc = rdtsc();
        struct { u64 tsc; u8 term; } string;
        string.tsc = tsc;
        string.term = 0;

        vga_print((u8*) &string);
    */

    u8 buffer[2];
    vga.attribute = 0x0f;

    Rand rand;
    rand_set_seed(&rand, rdtsc().lower);

    vga.cursor += 400;
    vga_newline();
    buffer[1] = 0;

    for (size n = 0; n < vga_rows - 6; n++) {
        u32 i = rand_range(&rand, 6, 8);
        buffer[0] = '0' + i;
        vga_print(buffer);
        // vga_newline();
    }

    while (true);
}


#pragma once

#include "common.h"

size str_length(u8*);
bool str_compare(u8*, u8*);
u8*  str_tokenize(u8*, u8);
bool str_startswith(u8*, u8*);

void port_write(u16, u8);
u8   port_read(u16);

void port_write_u16(u16, u16);
u16  port_read_u16(u16);

void io_wait();

struct Rand {
    u32 state;
};

struct u64 rdtsc();


struct Kbd_Key;
typedef void(*Kbd_Handler)(struct Kbd_Key*);
typedef void(*Vga_Spill_Handler)();

struct Akerios_Mode {
    Kbd_Handler key_handler;
    Vga_Spill_Handler scroll_handler;
    u8* title;
};
struct Akerios_Mode akerios_current_mode;

void clear_screen();
void pause(size ticks);

u32 rand_next_int(struct Rand*);
u32 rand_range(struct Rand*, u32, u32);
void rand_set_seed(struct Rand*, u32);

void assert(bool, u8*);


#pragma once

#include "common.h"

size str_length(u8*);
bool str_compare(u8*, u8*);

void port_write(u16, u8);
u8   port_read(u16);

void port_write_u16(u16, u16);
u16  port_read_u16(u16);

void io_wait();


struct Rand {
    u32 state;
};

struct u64 rdtsc();
void pause(size ticks);

u32 rand_next_int(struct Rand*);
u32 rand_range(struct Rand*, u32, u32);
void rand_set_seed(struct Rand*, u32);


#pragma once

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;

typedef u32      size;

#define if_not(cond) if (!(cond))

typedef enum {
    false, true
} bool;


size strlen(u8*);

void port_write(u16 port, u8 value);
u8   port_read(u16 port);

void port_write_u16(u16 port, u16 value);
u16  port_read_u16(u16 port);

void io_wait();

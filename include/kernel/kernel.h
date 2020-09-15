
#pragma once

#include "common.h"

size str_length(u8*);

void port_write(u16 port, u8 value);
u8   port_read(u16 port);

void port_write_u16(u16 port, u16 value);
u16  port_read_u16(u16 port);

void io_wait();

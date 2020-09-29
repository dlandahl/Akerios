
#pragma once

#include "common.h"

#define vga_cols 80
#define vga_rows 25
typedef void(*Vga_Spill_Handler)();

struct {
    u8 attribute;
    u8 back_attribute;
    u8* framebuffer;
    size cursor;
    size tab_stop;
    size framebuffer_size;
    Vga_Spill_Handler spill_handler;
} vga;

void vga_set_attribute(u8, size, bool);
void vga_hide_cursor();
void vga_move_cursor();
void vga_init();
void vga_newline();
void vga_tab();
void vga_clear();
void vga_print(i8*);
void vga_put(i8*, size, size);
void vga_print_char(i8);
void vga_print_hex(u32);
void vga_print_byte(u8);
void vga_invert();

void vga_set_spill_handler(Vga_Spill_Handler);
Vga_Spill_Handler vga_get_spill_handler();

enum Vga_Colour {
    vga_black,
    vga_blue,
    vga_green,
    vga_cyan,
    vga_red,
    vga_magenta,
    vga_brown,
    vga_light_grey,
    vga_grey,
    vga_light_blue,
    vga_light_green,
    vga_light_cyan,
    vga_light_red,
    vga_light_magenta,
    vga_light_yellow,
    vga_white,
};

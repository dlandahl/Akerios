
#pragma once

#define vga_cols 80
#define vga_rows 35

struct {
    u8 attribute;
    u8* framebuffer;
    size cursor;
    size tab_stop;
    size framebuffer_size;
} vga;

void vga_set_attribute(u8, size, bool);
void vga_hide_cursor();
void vga_move_cursor();
void vga_init();
void vga_newline();
void vga_tab();
void vga_clear();
void vga_print(u8*);
void vga_put(u8*, size, size);
void vga_print_char(u8);
void vga_print_hex(u32);

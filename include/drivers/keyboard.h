
#pragma once

enum Kbd_Scan_Code {
    key_none, key_esc,
    key_num1, key_num2, key_num3, key_num4, key_num5, key_num6, key_num7, key_num8, key_num9, key_num0,
    key_dash, key_equal, key_backspace, key_tab,
    key_q, key_w, key_e, key_r, key_t, key_y, key_u, key_i, key_o, key_p,
    key_open_bracket, key_closed_bracket, key_enter, key_ctrl,
    key_a, key_s, key_d, key_f, key_g, key_h, key_j, key_k, key_l,
    key_semicolon, key_apostrophe, key_backtick, key_lshift, key_backslash,
    key_z, key_x, key_c, key_v, key_b, key_n, key_m,
    key_comma, key_period, key_forwardslash, key_rshift, key_asterisk, key_alt, key_space, key_capslock,
    key_f1, key_f2, key_f3, key_f4, key_f5, key_f6, key_f7, key_f8, key_f9, key_f10,
    key_numlock
};

struct Kbd_Key {
    enum Kbd_Scan_Code scan_code;
    bool is_alphanumeric;
    bool shift, ctrl;
    bool is_release;
    u8 ascii;
};

typedef void(*Kbd_Handler)(struct Kbd_Key) ;

void kbd_init();
void kbd_set_handler(Kbd_Handler);
Kbd_Handler kbd_get_handler();

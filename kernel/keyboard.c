
#include "kernel.h"
#include "interrupts.h"
#include "keyboard.h"

struct {
    bool shift;
    bool ctrl;
    void(*handler)(struct Kbd_Key);
} kbd_state;

#define kbd_release 0x80

static const u8 kbd_scan_table[62] = "\0#1234567890-=\0\tqwertyuiop[]\0\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 ";

bool kbd_key_is_number(enum Kbd_Scan_Code code) {
    u8 character = kbd_scan_table[code];
    if (character >= '0' && character <= '9') return true;
    return false;
}

bool kbd_key_is_letter(enum Kbd_Scan_Code code) {
    u8 character = kbd_scan_table[code];
    if (character >= 'a' && character <= 'z') return true;
    return false;
}

__attribute__((interrupt))
void kbd_isr(struct Interrupt_Frame* frame) {

    enum Kbd_Scan_Code scan_code = port_read(0x60);
    struct Kbd_Key key;

    switch (scan_code) {
        case key_lshift:
        case key_rshift:
            kbd_state.shift = true;
            break;

        case key_lshift + kbd_release:
        case key_rshift + kbd_release:
            kbd_state.shift = false;
            break;

        case key_ctrl:
            kbd_state.ctrl = true;
            break;

        case key_ctrl + kbd_release:
            kbd_state.ctrl = false;
            break;
    }

    if (scan_code >= kbd_release) {
        key.is_release = true;
        scan_code -= kbd_release;
    }
    else key.is_release = false;

    key.scan_code = scan_code;

    if (key.scan_code < sizeof(kbd_scan_table))
        key.ascii = kbd_scan_table[key.scan_code];
    else key.ascii = 0;

    if (kbd_key_is_number(scan_code)) {
        key.is_alphanumeric = true;
        if (kbd_state.shift) key.ascii -= 16;
    }
    else if (kbd_key_is_letter(scan_code)) {
        key.is_alphanumeric = true;
        if (kbd_state.shift) key.ascii -= 32;
    }
    else key.is_alphanumeric = false;

    key.shift = kbd_state.shift;
    key.ctrl  = kbd_state.ctrl;

    if (kbd_state.handler) kbd_state.handler(key);
    pic_send_eoi(irq_kbd);
    return;
}

void kbd_init() {
    idt_add_entry(&kbd_isr, 0x21);
    kbd_state.handler = 0;
    pic_mask(irq_kbd, true);
}

void kbd_set_handler(void(*handler)(struct Kbd_Key)) {
    kbd_state.handler = handler;
}

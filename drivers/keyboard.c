
#include "kernel/kernel.h"
#include "kernel/interrupts.h"
#include "drivers/keyboard.h"

struct {
    bool shift;
    bool ctrl;
    Kbd_Handler handler;
} kbd_state;

#define kbd_release 0x80

static const u8 kbd_scan_table[62]
    = "\0#1234567890-=\0\tqwertyuiop[]\0\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 ";
static const u8 kbd_scan_table_shifted[62]
    = "\0#!@#$%^&*()_+\0\0QWERTYUIOP{}\0\0ASDFGHJKL:\"~\0|ZXCVBNM<>?\0*\0 ";

internal bool kbd_key_is_number(enum Kbd_Scan_Code code) {
    u8 character = kbd_scan_table[code];
    if (character >= '0' && character <= '9') return true;
    return false;
}

internal bool kbd_key_is_letter(enum Kbd_Scan_Code code) {
    u8 character = kbd_scan_table[code];
    if (character >= 'a' && character <= 'z') return true;
    return false;
}

__attribute__((interrupt))
internal void kbd_isr(struct Interrupt_Frame* frame) {
    asm("cli");
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
        default: break;
    }

    if (scan_code >= kbd_release) {
        key.is_release = true;
        scan_code -= kbd_release;
    }
    else key.is_release = false;

    key.scan_code = scan_code;

    if (scan_code < size_of(kbd_scan_table)) {
        if (kbd_state.shift) key.ascii = kbd_scan_table_shifted[scan_code];
        else                 key.ascii = kbd_scan_table[scan_code];
    }
    else key.ascii = 0;

    if (kbd_key_is_number(scan_code) || kbd_key_is_letter(scan_code)) {
        key.is_alphanumeric = true;
    }
    else key.is_alphanumeric = false;

    key.shift = kbd_state.shift;
    key.ctrl  = kbd_state.ctrl;

    if (akerios_current_mode.key_handler) akerios_current_mode.key_handler(&key);
    pic_send_eoi(irq_kbd);
    asm("sti");
    return;
}

void kbd_init() {
    idt_add_entry(&kbd_isr, 0x21);
    kbd_state.handler = 0;
    pic_mask(irq_kbd, true);
}

void kbd_set_handler(Kbd_Handler handler) {
    kbd_state.handler = handler;
}

Kbd_Handler kbd_get_handler() {
    return kbd_state.handler;
}

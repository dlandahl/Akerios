
#pragma once

struct Interrupt_Frame;

void idt_add_entry(void*, size);
void idt_init();

enum {
    irq_time = 0x0,
    irq_kbd  = 0x1,
};

void pic_send_eoi(u8);
void pic_init();
void pic_mask(u8, bool);

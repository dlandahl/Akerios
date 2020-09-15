
#include "kernel.h"
#include "interrupts.h"

struct Interrupt_Frame {
    u32 ip;
    u32 cs;
    u32 flags;
    u32 sp;
    u32 ss;
};

struct Idt_Entry {
   u16 offset_1;
   u16 selector;
   u8  zero;
   u8  type_attr;
   u16 offset_2;
} __attribute__((packed));

struct {
    u16 limit;
    u32 base;
} __attribute__((packed)) idt_desc;

struct Idt_Entry idt[0x100] = { };

void idt_add_entry(void* addr, size index) {
    struct Idt_Entry entry;
    entry.offset_1  = (u32) addr;
    entry.offset_2  = (u32) addr >> 16;
    entry.selector  = 0b00001000;
    entry.zero      = 0x0;
    entry.type_attr = 0xee;
    idt[(index)]    = entry;
}

void idt_init() {
    idt_desc.limit = 0x100 * size_of(struct Idt_Entry);
    idt_desc.base  = (u32) idt;

    asm("lidt %0" :: "m"(idt_desc));
}



enum {
    pic_port_master_cmd  = 0x20,
    pic_port_master_data = 0x21,
    pic_port_slave_cmd   = 0xa0,
    pic_port_slave_data  = 0xa1,
};

enum {
    pic_cmd_eoi  = 0x20,
    pic_cmd_init = 0x11,
};

void pic_send_eoi(u8 irq) {
    if (irq > 7) {
        port_write(pic_port_slave_cmd, pic_cmd_eoi);
    }
    port_write(pic_port_master_cmd, pic_cmd_eoi);
}

void pic_init() {
    u8 ICW2[2] = { 0x20, 0x28 };
    u8 ICW3[2] = { 4, 2 };

    const int cmd_port  = pic_port_master_cmd;
    const int data_port = pic_port_master_data;
    for (size n = 0; n < 2; n++) {
        const int select = (pic_port_slave_cmd - pic_port_master_cmd) * n;

        u8 mask = port_read(pic_port_master_data + select);
        port_write(cmd_port + select, pic_cmd_init);
        port_write(data_port + select, ICW2[n]);
        port_write(data_port + select, ICW3[n]);
        port_write(data_port + select, 1);
        port_write(data_port + select, mask);

        port_write(data_port + select, 0xff);
    }
    asm("sti");
}

void pic_mask(u8 irq, bool unmask) {
    u16 port = pic_port_master_data;

    if (irq > 7) {
        irq -= 8;
        port = pic_port_slave_data;
    }

    const u8 bit = 1 << irq;
    u8 value = unmask ? (port_read(port) & ~bit)
                      : (port_read(port) |  bit);
    port_write(port, value);        
}

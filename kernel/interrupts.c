
#include "drivers/vga.h"
#include "kernel/kernel.h"
#include "kernel/interrupts.h"

struct Interrupt_Frame {
    u32 ip;
    u32 cs;
    u32 flags;
    u32 sp;
    u32 ss;
};

__attribute__((interrupt))
void isr_zero_division(struct Interrupt_Frame* frame) {
    vga_print("\nZero division fault, halting.\n");
    interrupt_print_frame(frame);
    while (true);
}

__attribute__((interrupt))
void isr_debug(struct Interrupt_Frame* frame) {
    vga_print("\nDebug exception.\n");
    interrupt_print_frame(frame);
    while (true);
}

__attribute__((interrupt))
void isr_nmi(struct Interrupt_Frame* frame) {
    vga_print("\nNon-maksable Interrupt exception.\n");
    interrupt_print_frame(frame);
    while (true);
}

__attribute__((interrupt))
void isr_breakpoint(struct Interrupt_Frame* frame) {
    vga_print("\nBreakpoint exception.\n");
    interrupt_print_frame(frame);
    while (true);
}

__attribute__((interrupt))
void isr_overflow(struct Interrupt_Frame* frame) {
    vga_print("\nOverflow exception.\n");
    interrupt_print_frame(frame);
    while (true);
}

__attribute__((interrupt))
void isr_bound_range(struct Interrupt_Frame* frame) {
    vga_print("\nBound range exceeded exception.\n");
    interrupt_print_frame(frame);
    while (true);
}

__attribute__((interrupt))
void isr_invalid_opcode(struct Interrupt_Frame* frame) {
    vga_print("\nInvalid opcode exception.\n");
    interrupt_print_frame(frame);
    while (true);
}

__attribute__((interrupt))
void isr_device(struct Interrupt_Frame* frame) {
    vga_print("\nDevice not available exception.\n");
    interrupt_print_frame(frame);
    while (true);
}

__attribute__((interrupt))
void isr_double_fault(struct Interrupt_Frame* frame) {
    vga_print("\nDouble Fault.\n");
    interrupt_print_frame(frame);
    while (true);
}

__attribute__((interrupt))
void isr_invalid_tss(struct Interrupt_Frame* frame) {
    vga_print("\nInvalid TSS.\n");
    interrupt_print_frame(frame);
    while (true);
}

__attribute__((interrupt))
void isr_segment(struct Interrupt_Frame* frame) {
    vga_print("\nSegment not present.\n");
    interrupt_print_frame(frame);
    while (true);
}

__attribute__((interrupt))
void isr_stack_fault(struct Interrupt_Frame* frame) {
    vga_print("\nStack segment fault.\n");
    interrupt_print_frame(frame);
    while (true);
}

__attribute__((interrupt))
void isr_general_prodection(struct Interrupt_Frame* frame) {
    vga_print("\nGeneral protection fault.\n");
    interrupt_print_frame(frame);
    while (true);
}

__attribute__((interrupt))
void isr_page_fault(struct Interrupt_Frame* frame) {
    vga_print("\nPage fault.\n");
    interrupt_print_frame(frame);
    while (true);
}

__attribute__((interrupt))
void isr_float(struct Interrupt_Frame* frame) {
    vga_print("\nFloating point exception.\n");
    interrupt_print_frame(frame);
    while (true);
}

__attribute__((interrupt))
void isr_alignment(struct Interrupt_Frame* frame) {
    vga_print("\nAlignment check fault.\n");
    interrupt_print_frame(frame);
    while (true);
}

__attribute__((interrupt))
void isr_machine(struct Interrupt_Frame* frame) {
    vga_print("\nMachine check fault.\n");
    interrupt_print_frame(frame);
    while (true);
}

__attribute__((interrupt))
void isr_simd(struct Interrupt_Frame* frame) {
    vga_print("\nSIMD Float exception fault.\n");
    interrupt_print_frame(frame);
    while (true);
}

__attribute__((interrupt))
void isr_virt(struct Interrupt_Frame* frame) {
    vga_print("\nVirtualization exception fault.\n");
    interrupt_print_frame(frame);
    while (true);
}

__attribute__((interrupt))
void isr_sec(struct Interrupt_Frame* frame) {
    vga_print("\nSecurity exception fault.\n");
    interrupt_print_frame(frame);
    while (true);
}



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
    entry.type_attr = 0x8e;
    idt[(index)]    = entry;
}

void idt_init() {
    idt_desc.limit = 0x100 * size_of(struct Idt_Entry);
    idt_desc.base  = (u32) idt;

    idt_add_entry(&isr_zero_division, 0x0);
    idt_add_entry(&isr_debug, 0x1);
    idt_add_entry(&isr_nmi, 0x2);
    idt_add_entry(&isr_breakpoint, 0x3);
    idt_add_entry(&isr_overflow, 0x4);
    idt_add_entry(&isr_bound_range, 0x5);
    idt_add_entry(&isr_invalid_opcode, 0x6);
    idt_add_entry(&isr_device, 0x7);
    idt_add_entry(&isr_double_fault, 0x8);
    idt_add_entry(&isr_invalid_tss, 0xa);
    idt_add_entry(&isr_segment, 0xb);
    idt_add_entry(&isr_stack_fault, 0xc);
    idt_add_entry(&isr_general_prodection, 0xd);
    idt_add_entry(&isr_page_fault, 0xe);
    idt_add_entry(&isr_float, 0x10);
    idt_add_entry(&isr_alignment, 0x11);
    idt_add_entry(&isr_machine, 0x12);
    idt_add_entry(&isr_simd, 0x13);
    idt_add_entry(&isr_virt, 0x14);
    idt_add_entry(&isr_sec, 0x1e);

    asm("lidt %0" :: "m"(idt_desc));
}

void interrupt_print_frame(struct Interrupt_Frame* frame) {
    vga_print("\t|IP: ");
    vga_print_hex(frame->ip);
    vga_print("|\t\t");

    vga_print("|CS: ");
    vga_print_hex(frame->cs);
    vga_print("|\t\t");

    vga_print("|FLAGS: ");
    vga_print_byte(frame->flags);
    vga_print("|\n\t");

    vga_print("|SP: ");
    vga_print_hex(frame->sp);
    vga_print("|\t\t");

    vga_print("|SS: ");
    vga_print_hex(frame->ss);
    vga_print("|\t\t");
    vga_hide_cursor();
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

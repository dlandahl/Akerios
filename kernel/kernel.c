
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "kernel/interrupts.h"
#include "kernel/shell.h"
#include "kernel/filesystem.h"
#include "drivers/keyboard.h"
#include "drivers/vga.h"
#include "drivers/ata.h"



size str_length(u8* string) {
    size n;
    for (n = 0; string[n]; n++);
    return n;
}

bool str_compare(u8* a, u8* b) {
    for (size n = 0; a[n] || b[n]; n++) {
        if (a[n] != b[n]) return false;
    }
    return true;
}

void port_write(u16 port, u8 value) {
    asm("out dx, al" :: "a"(value), "d"(port));
}

u8 port_read(u16 port) {
    u8 value;
    asm("in al, dx" : "=a"(value) : "d"(port));
    return value;
}

void port_write_u16(u16 port, u16 value) {
    asm("out dx, ax" :: "a"(value),  "d"(port));
}

u16 port_read_u16(u16 port) {
    u16 value;
    asm("in ax, dx" : "=a"(value) : "d"(port));
    return value;
}

void io_wait() {
    asm("out 0x80, al" :: "a"(0));
}



struct u64 rdtsc() {
    struct u64 tsc;
    asm("rdtsc" : "=d"(tsc.upper), "=a"(tsc.lower));
    return tsc;
}



#define pit_dividend 1193182

enum {
    pit_port_channel_0 = 0x40,
    pit_port_channel_1 = 0x41,
    pit_port_channel_2 = 0x42,
    pit_port_command   = 0x43,
};

void pit_set_divisor(u32 divisor) {
    port_write(pit_port_command, 0x36);
    port_write(pit_port_channel_0, (u8) (divisor      & 0xff));
    port_write(pit_port_channel_0, (u8) (divisor >> 8 & 0xff));
}

u16 pit_set_frequency(u32 frequency) {
    u16 divisor = pit_dividend / frequency;
    pit_set_divisor(divisor);
    return divisor;
}



u32 rand_next_int(struct Rand* rand) {
    u32 x = rand->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return rand->state = x;
}

u32 rand_range(struct Rand* rand, u32 min, u32 max) {
    u32 value = rand_next_int(rand);
    value %= max - min;
    return value + min + 1;
}

void rand_set_seed(struct Rand* rand, u32 seed) {
    rand->state = seed;
}




__attribute__((interrupt))
void isr_zero_division(struct Interrupt_Frame* frame) {
    vga_print("\nZero division fault, halting.\n");
    interrupt_print_frame(frame);
    asm("hlt");
}
__attribute__((interrupt))
void isr_debug(struct Interrupt_Frame* frame) {
    vga_print("\nDebug exception.\n");
    interrupt_print_frame(frame);
    asm("hlt");
}

__attribute__((interrupt))
void isr_nmi(struct Interrupt_Frame* frame) {
    vga_print("\nNon-maksable Interrupt exception.\n");
    interrupt_print_frame(frame);
    asm("hlt");
}

__attribute__((interrupt))
void isr_breakpoint(struct Interrupt_Frame* frame) {
    vga_print("\nBreakpoint exception.\n");
    interrupt_print_frame(frame);
    asm("hlt");
}

__attribute__((interrupt))
void isr_overflow(struct Interrupt_Frame* frame) {
    vga_print("\nOverflow exception.\n");
    interrupt_print_frame(frame);
    asm("hlt");
}

__attribute__((interrupt))
void isr_bound_range(struct Interrupt_Frame* frame) {
    vga_print("\nBound range exceeded exception.\n");
    interrupt_print_frame(frame);
    asm("hlt");
}

__attribute__((interrupt))
void isr_invalid_opcode(struct Interrupt_Frame* frame) {
    vga_print("\nInvalid opcode exception.\n");
    interrupt_print_frame(frame);
    asm("hlt");
}

__attribute__((interrupt))
void isr_device(struct Interrupt_Frame* frame) {
    vga_print("\nDevice not available exception.\n");
    interrupt_print_frame(frame);
    asm("hlt");
}

__attribute__((interrupt))
void isr_double_fault(struct Interrupt_Frame* frame) {
    vga_print("\nDouble Fault.\n");
    interrupt_print_frame(frame);
    asm("hlt");
}

__attribute__((interrupt))
void isr_invalid_tss(struct Interrupt_Frame* frame) {
    vga_print("\nInvalid TSS.\n");
    interrupt_print_frame(frame);
    asm("hlt");
}

__attribute__((interrupt))
void isr_segment(struct Interrupt_Frame* frame) {
    vga_print("\nSegment not present.\n");
    interrupt_print_frame(frame);
    asm("hlt");
}

__attribute__((interrupt))
void isr_stack_fault(struct Interrupt_Frame* frame) {
    vga_print("\nStack segment fault.\n");
    interrupt_print_frame(frame);
    asm("hlt");
}

__attribute__((interrupt))
void isr_general_prodection(struct Interrupt_Frame* frame) {
    vga_print("\nGeneral protection fault.\n");
    interrupt_print_frame(frame);
    asm("hlt");
}

__attribute__((interrupt))
void isr_page_fault(struct Interrupt_Frame* frame) {
    vga_print("\nPage fault.\n");
    interrupt_print_frame(frame);
    asm("hlt");
}

__attribute__((interrupt))
void isr_float(struct Interrupt_Frame* frame) {
    vga_print("\nFloating point exception.\n");
    interrupt_print_frame(frame);
    asm("hlt");
}

__attribute__((interrupt))
void isr_alignment(struct Interrupt_Frame* frame) {
    vga_print("\nAlignment check fault.\n");
    interrupt_print_frame(frame);
    asm("hlt");
}

__attribute__((interrupt))
void isr_machine(struct Interrupt_Frame* frame) {
    vga_print("\nMachine check fault.\n");
    interrupt_print_frame(frame);
    asm("hlt");
}

__attribute__((interrupt))
void isr_simd(struct Interrupt_Frame* frame) {
    vga_print("\nSIMD Float exception fault.\n");
    interrupt_print_frame(frame);
    asm("hlt");
}

__attribute__((interrupt))
void isr_virt(struct Interrupt_Frame* frame) {
    vga_print("\nVirtualization exception fault.\n");
    interrupt_print_frame(frame);
    asm("hlt");
}

__attribute__((interrupt))
void isr_sec(struct Interrupt_Frame* frame) {
    vga_print("\nSecurity exception fault.\n");
    interrupt_print_frame(frame);
    asm("hlt");
}


void timer_kbd_void(struct Kbd_Key key) {
    return;
}

volatile size counter;
void pause(size ticks) {
    ticks *= 11932;
    size freeze = counter;
    Kbd_Handler old_handler = kbd_get_handler();
    kbd_set_handler(&timer_kbd_void);
    while (counter - freeze < ticks);
    kbd_set_handler(old_handler);
}

__attribute__((interrupt))
void isr_timer(struct Interrupt_Frame* frame) { 
    asm("cli");
    counter++;
    pic_send_eoi(irq_time);
    asm("sti");
}

void assert(bool condition, u8* message) {
    if (!condition) {
        vga_print("KERNEL ERROR\n");
        if (message) vga_print(message);
        asm("hlt");
        while (true);
    }
}

void shell_test() {
    heap_init();
    vga_init();
    vga.attribute = 0xf0;
    idt_init();
    pic_init();
    // vga_hide_cursor();

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

    // idt_add_entry(&isr_timer, 0x20);
    // pic_mask(irq_time, true);

    kbd_init();
    shell_init();

    // vga_clear();
    // vga_print("Hello!");
    pit_set_divisor(100);

    // vga_print(msg_welcome);

    // u16* a = heap_allocate_typed(u16, 1);
    // int* b = heap_allocate_typed(int, 32);
    // vga_print_hex(cast(int, b));
    while (true);
}

void kernel_entry() {
    shell_test();
    heap_init();
    vga_init();
    vga.attribute = 0x4e;

    vga_clear();

    vga.tab_stop = 12;
    Ata_Error err = fs_format();
    fs_create_file("history");
    fs_create_file("log");

    size file_size = fs_blocksize + 2048;
    u8* data = heap_allocate(file_size);
    mem_clear(data, file_size);
    *cast(u32*, data) = 0x1234abcd;

    bool success = fs_write_entire_file("history", data, file_size);
    vga_print("File write success: ");
    vga_print(success ? "success\n" : "fail\n");

    mem_clear(data, file_size);
    heap_deallocate(data);
    data = fs_read_entire_file("history");
    vga_newline();
    vga_print_hex(*cast(u32*, data));
    heap_deallocate(data);

    vga_newline();
    vga_newline();
    fs_list_directory();
    Fat_Entry* fat = heap_allocate(fs_blocksize);
    ata_lba_read(fat, fs_disk_location, 1);

    vga_newline();
    for (size n = 0; n < 18; n++) {
        vga_print_hex(fat[n]);
        vga_print(" | ");
        if_not((n+1) % 6) vga_newline();
    }
    while (true);
}

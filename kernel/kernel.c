
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "kernel/interrupts.h"
#include "kernel/shell.h"
#include "drivers/keyboard.h"
#include "drivers/vga.h"
#include "drivers/ata.h"


size str_length(u8* string) {
    size n;
    for (n = 0; string[n]; n++);
    return n;
}

bool str_compare(u8* a, u8* b) {
    for (size n = 0; a[n] || b[n]; n++)
        if (a[n] != b[n]) return false;

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



struct Rand {
    u32 state;
};

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



u8* msg_welcome =
"  /%%%%%%  /%%                           /%%  /%%%%%%   /%%%%%% \n"
" /%%__  %%| %%                          |__/ /%%__  %% /%%__  %%\n"
"| %%  \\ %%| %%   /%%  /%%%%%%   /%%%%%%  /%%| %%  \\ %%| %%  \\__/\n"
"| %%%%%%%%| %%  /%%/ /%%__  %% /%%__  %%| %%| %%  | %%|  %%%%%% \n"
"| %%__  %%| %%%%%%/ | %%%%%%%%| %%  \\__/| %%| %%  | %% \\____  %%\n"
"| %%  | %%| %%_  %% | %%_____/| %%      | %%| %%  | %% /%%  \\ %%\n"
"| %%  | %%| %% \\  %%|  %%%%%%%| %%      | %%|  %%%%%%/|  %%%%%%/\n"
"|__/  |__/|__/  \\__/ \\_______/|__/      |__/ \\______/  \\______/ \n\n";


u8* current_program = "Shell";



__attribute__((interrupt))
void isr_zero_division(struct Interrupt_Frame* frame) {
    vga_print("\nZero division fault, halting.\n");
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
    counter++;
    pic_send_eoi(irq_time);
}

void clear_screen() {
    vga_clear();
    vga_print(msg_welcome);
    vga_print("=== ");
    vga_print(current_program);
    vga_print_char(' ');
    for (int n = 0; n < vga_cols - 5 - str_length(current_program); n++)
        vga_print_char('=');
}

void shell_test() {
    heap_init();
    vga_init();
    vga.attribute = 0x0b;
    idt_init();
    pic_init();
    // vga_hide_cursor();

    idt_add_entry(&isr_zero_division, 0x0);
    idt_add_entry(&isr_timer, 0x20);
    pic_mask(irq_time, true);

    kbd_init();
    clear_screen();
    shell_init();

    pit_set_divisor(100);

    // vga_print(msg_welcome);

    // u16* a = heap_allocate_typed(u16, 1);
    // int* b = heap_allocate_typed(int, 32);
    // vga_print_hex(cast(int, b));
    while (true);
}

void kernel_entry() {
    heap_init();
    vga_init();
    vga.attribute = 0x0b;

    vga_clear();
    vga_hide_cursor();
    vga_print(msg_welcome);

    u32* buffer = heap_allocate_typed(u32, 128);
    ata_lba_read(128, 1, buffer);

    vga_print_hex(buffer[0]);
    while (true);
}

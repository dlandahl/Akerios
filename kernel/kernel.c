
#include "kernel.h"
#include "interrupts.h"
#include "vga.h"


size strlen(u8* string) {
    size n;
    for (n = 0; string[n]; n++);
    return n;
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


struct u64 {
    u32 lower, upper;
} __attribute__((packed));

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
    rand->state = x;
    return x;
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
"  /$$$$$$          /$$                          /$$     /$$$$$$  /$$$$$$ \n"
" /$$__  $$        | $$                         | $$    /$$__  $$/$$__  $$\n"
"| $$  \\ $$ /$$$$$$| $$$$$$$  /$$$$$$  /$$$$$$$/$$$$$$ | $$  \\ $| $$  \\__/\n"
"| $$$$$$$$/$$_____| $$__  $ /$$__  $$/$$_____|_  $$_/ | $$  | $|  $$$$$$ \n"
"| $$__  $|  $$$$$$| $$  \\ $| $$$$$$$|  $$$$$$  | $$   | $$  | $$\\____  $$\n"
"| $$  | $$\\____  $| $$  | $| $$_____/\\____  $$ | $$ /$| $$  | $$/$$  \\ $$\n"
"| $$  | $$/$$$$$$$| $$$$$$$|  $$$$$$$/$$$$$$$/ |  $$$$|  $$$$$$|  $$$$$$/\n"
"|__/  |__|_______/|_______/ \\_______|_______/   \\___/  \\______/ \\______/ \n"
"\n  Welcome\n"
"\n\t\tWe like to have fun here\n";


struct Stack {
    u32 top, bottom;
};

void stack_init(struct Stack* stack, u32 location) {
    stack->top = stack->bottom = location;
}

void stack_push(struct Stack* stack, u32 value) {
    **(u32**) (&stack->top) = value;
    stack->top += 4;
}

u32 stack_pop(struct Stack* stack) {
    stack->top -= 4;
    return **(u32**) (&stack->top);
}


__attribute__((interrupt))
void isr_zero_division(struct Interrupt_Frame* frame) {
    vga_print("Zero division fault\n");
    asm("hlt");
}

__attribute__((interrupt))
void isr_timer(struct Interrupt_Frame* frame) {
    static size counter;
    static size digit;
    u8 buffer[2] = { 0 };
    if_not (counter++ % 18) {
        buffer[0] = '0' + (digit++ % 10);
        vga_put(buffer, 5, 2);
    }
    pic_send_eoi(irq_time);
}

struct {
    bool shift;
    bool ctrl;
    bool alt;
} keyboard;

#define key_release 0x80

enum Scan_Code {
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

__attribute__((interrupt))
void isr_keyboard(struct Interrupt_Frame* frame) {

    static const u8 scan_code[] = "  1234567890-= \tqwertyuiop[]  asdfghjkl;'` \\zxcvbnm,./ *  ";

    u8 key_code = port_read(0x60);

    switch (key_code) {
    case key_none:
    case key_esc:
    case 0x1c:
    case 0x1d:
        break;

    case key_lshift:
    case key_rshift:
        keyboard.shift = true;
        break;

    case key_lshift + key_release:
    case key_rshift + key_release:
        keyboard.shift = false;
        break;

    case key_backspace:
        vga.cursor--;
        vga_print_char(' ');
        vga.cursor--;
        break;

    default:
        if (key_code >= 0x80) break;
        u8 character = scan_code[key_code];
        if (keyboard.shift) {
            if (character >= '0' && character <= '9') character -= 16;
            else character -= 32;
        }
        vga_print_char(character);
    }

    pic_send_eoi(irq_kbd);
    return;
}



bool shell;

void kernel_entry() {
    vga_init();
    idt_init();
//  pit_set_divisor(0xffffff);
    pic_init();
//
//  shell = true;
//
    idt_register_entry(&isr_timer, 0x20);
    pic_mask(irq_time, true);
    idt_register_entry(&isr_keyboard, 0x21);
    pic_mask(irq_kbd,  true);
//
    vga_clear();
    vga_print(msg_welcome);
//
//  u32 value = 0xabcd1234;
//  vga_print_hex(value);
//  vga_newline();

    while (true);
}

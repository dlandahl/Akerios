
#include "kernel.h"
#include "interrupts.h"
#include "keyboard.h"
#include "vga.h"


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



struct u64 {
    u32 lower, upper;
} __attribute__((packed));

struct u64 rdtsc() {
    struct u64 tsc;
    asm("rdtsc" : "=d"(tsc.upper), "=a"(tsc.lower));
    return tsc;
}



void sprint_hex(u32 number) {
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
"  /%%%%%%  /%%                           /%%  /%%%%%%   /%%%%%% \n"
" /%%__  %%| %%                          |__/ /%%__  %% /%%__  %%\n"
"| %%  \\ %%| %%   /%%  /%%%%%%   /%%%%%%  /%%| %%  \\ %%| %%  \\__/\n"
"| %%%%%%%%| %%  /%%/ /%%__  %% /%%__  %%| %%| %%  | %%|  %%%%%% \n"
"| %%__  %%| %%%%%%/ | %%%%%%%%| %%  \\__/| %%| %%  | %% \\____  %%\n"
"| %%  | %%| %%_  %% | %%_____/| %%      | %%| %%  | %% /%%  \\ %%\n"
"| %%  | %%| %% \\  %%|  %%%%%%%| %%      | %%|  %%%%%%/|  %%%%%%/\n"
"|__/  |__/|__/  \\__/ \\_______/|__/      |__/ \\______/  \\______/ \n\n";


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

size counter;
__attribute__((interrupt))
void isr_timer(struct Interrupt_Frame* frame) {
    counter++;
    pic_send_eoi(irq_time);
}


void wait() {
    // asm("hlt");
    // size start = counter;
    // while (true) {
    //     if (counter - start) break;
    // };
}

void random() {
    struct Rand rand;
    rand_set_seed(&rand, rdtsc().lower);
    size value = rand_next_int(&rand);
    vga_print_hex(value);
}

void time() {
    vga_print("Lower: ");
    vga_print_hex(rdtsc().lower);
    vga_print("\nUpper: ");
    vga_print_hex(rdtsc().upper);
}

void hello_world() {
    vga_print("Hello, World!");
}

void clear_screen() {
    vga_clear();
    vga_print(msg_welcome);
    vga_print("=== SHELL ============================================================");
}

void colour() {
    struct u64 tsc;
    asm("rdtsc" : "=d"(tsc.upper), "=a"(tsc.lower));
    u8 x = tsc.lower & 0xff;
    vga.attribute = x;
    for (size n = 0; n < vga.framebuffer_size; n++) {
        vga.framebuffer[n*2+1] = x;
    }
}


struct {
    u8 command_buffer[vga_cols - 2];
    size length;
} shell;

u8* shell_commands[6] = {
    "hello", "clear", "random", "rdtsc", "colour"
};

void(*shell_routines[])() = {
    &hello_world, &clear_screen, &random, &time, &colour, &wait
};

void shell_submit_command() {

//    vga_newline();
//    vga_print("Command is:");
//    vga_print(shell.command_buffer);

    bool found = false;
    for (size n = 0; n < sizeof(shell_commands); n++) {
        if (str_compare(shell.command_buffer, shell_commands[n])) {
            vga_newline();
            shell_routines[n]();
            found = true;
            break;
        }
    }

    if (!found && shell.length) {
        vga_print("\nCommand not recognised: ");
        vga_print(shell.command_buffer);
    }

    for (size n = 0; n < sizeof(shell.command_buffer); n++)
        shell.command_buffer[n] = 0;

    shell.length = 0;
    vga_newline();
    vga_print("# ");
    vga_move_cursor(vga.cursor);
}

void shell_keypress(struct Kbd_Key key) {
    if (key.is_release) return;

    if (key.ascii) {
        if (shell.length > 65) return;
        vga_print_char(key.ascii);
        shell.command_buffer[shell.length++] = key.ascii;
        vga_move_cursor(vga.cursor);
    }

    else switch (key.scan_code) {
        case key_backspace:
            if (vga.cursor % vga_cols <= 2) break;
            vga.cursor--;
            vga_print_char(' ');
            vga.cursor--;
            shell.length--;
            shell.command_buffer[shell.length] = 0;
            vga_move_cursor(vga.cursor);
            break;
        case key_enter:
            shell_submit_command();
    }
    return;

}

void kernel_entry() {
    counter = 0;
    vga_init();
    vga.attribute = 0x4f;
    idt_init();
    pic_init();
    // vga_hide_cursor();

    idt_add_entry(&isr_timer, 0x20);
    pic_mask(irq_time, true);

    clear_screen();
    vga_print("\n# ");
    vga_move_cursor(vga.cursor);
    kbd_init();
    kbd_set_handler(&shell_keypress);
}

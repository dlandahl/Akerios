
#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;

typedef u32      size;

#define if_not(cond) if (!(cond))

typedef enum {
    false, true
} bool;

size strlen(u8* string) {
    size n;
    for (n = 0; string[n]; n++);
    return n;
}

void port_write_u8(u16 port, u8 value) {
    asm("out dx, al" :: "a"(value), "d"(port));
}

u8 port_read_u8(u16 port) {
    u8 value;
    asm("in al, dx" : "=a"(value) : "d"(port));
    return value;
}

__attribute__((alias("port_read_u8" )))
u8   port_read  (u16);

__attribute__((alias("port_write_u8")))
void port_write (u16, u8);

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



struct Interrupt_Frame {
    u32 ip;
    u32 cs;
    u32 flags;
    u32 sp;
    u32 ss;
};

__attribute__((interrupt))
void isr_timer(struct Interrupt_Frame*);

__attribute__((interrupt))
void isr_zero_division(struct Interrupt_Frame*);

__attribute__((interrupt))
void isr_keyboard(struct Interrupt_Frame*);

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

#define IDT_ADD_ENTRY(name, index) {     \
    struct Idt_Entry entry;              \
    entry.offset_1  = (u32) &name;       \
    entry.offset_2  = (u32) &name >> 16; \
    entry.selector  = 0b00001000;        \
    entry.zero      = 0x0;               \
    entry.type_attr = 0xee;              \
    idt[(index)]    = entry;             \
}

void idt_init() {
    idt_desc.limit = 0x100 * sizeof(struct Idt_Entry);
    idt_desc.base  = (u32) idt;

    IDT_ADD_ENTRY(isr_zero_division, 0x0);
    IDT_ADD_ENTRY(isr_timer,         0x20);
    IDT_ADD_ENTRY(isr_keyboard,      0x21);
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

        // port_write(data_port + select, 0xff);
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



struct u64 {
    u32 lower, upper;
} __attribute__((packed));

struct u64 rdtsc() {
    struct u64 tsc;
    asm("rdtsc" : "=d"(tsc.upper), "=a"(tsc.lower));
    return tsc;
}



#define pit_dividend 1193182
#define pit_min_divisor 65536

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



#define vga_cols 80
#define vga_rows 35

struct {
    u8 attribute;
    u8* framebuffer;
    size cursor;
    size framebuffer_size;
} vga;

enum {
    vga_reg_input_status      = 0x3da,
    vga_reg_attribute_address = 0x3c0,
    vga_reg_data_read         = 0x3c1,
};

void vga_set_attribute(u8 register_index, size bit, bool unset) {
    const u8 PAS  = 1 << 5;
    const u8 mask = 1 << bit;

    port_read(vga_reg_input_status);
    const u8 address = port_read(vga_reg_attribute_address);
    port_write(vga_reg_attribute_address, register_index | PAS);

    u8 data = port_read(vga_reg_data_read);
    data = unset ? (data & ~mask)
                 : (data |  mask);
    port_write(vga_reg_attribute_address, data);

    port_write(vga_reg_attribute_address, address);
}

void vga_init() {
    vga.cursor = 0;
    vga.attribute = 0x83;
    vga.framebuffer = (u8*) 0xb8000;
    vga.framebuffer_size = vga_cols * vga_rows * sizeof(u16);

    vga_set_attribute(0x10, 3, true);
}

void vga_newline() {
    vga.cursor += vga_cols - vga.cursor % vga_cols;
}

void vga_clear() {
    for (size n = 0; n < vga.framebuffer_size / 2; n++) {
        vga.framebuffer[n*2]   = ' ';
        vga.framebuffer[n*2+1] = vga.attribute;
    }
    vga.cursor = 0;
}

void vga_print(u8* message) {
    for (size n = 0; message[n]; n++) {
        size index = vga.cursor * 2;

        switch (message[n]) {
            case '\n': vga_newline();   break;
            case '\t': vga.cursor += 4; break;
            default:
                vga.framebuffer[index]   = message[n];
                vga.framebuffer[index+1] = vga.attribute;
                vga.cursor++;
        }
    }
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


void kernel_entry() {
    vga_init();
    idt_init();
    pit_set_divisor(pit_min_divisor);
    pic_init();
    pic_mask(0x0, true);
    pic_mask(0x1, true);
    pic_mask(0x20, true);
    pic_mask(0x21, true);
    vga_clear();
    vga_print(msg_welcome);

    while (true);
}

__attribute__((interrupt))
void isr_zero_division(struct Interrupt_Frame* frame) {
    vga_print("Zero division fault\n");
    asm("hlt");
}

__attribute__((interrupt))
void isr_timer(struct Interrupt_Frame* frame) {
    static size counter;
    if_not (counter++ % 18) vga_print("Time...\n");
    pic_send_eoi(0x20);
}

__attribute__((interrupt))
void isr_keyboard(struct Interrupt_Frame* frame) {
    u8 buffer[2] = { 0 };
    buffer[0] = port_read(0x60);
    vga_print(buffer);

    pic_send_eoi(0x1);
}

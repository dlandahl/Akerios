
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "kernel/interrupts.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"

enum Token_Type {
    token_name,
    token_number,
    token_string,
    token_operator,
    token_comma,
    token_line_end,
    token_error,
};

enum Data_Type {
    type_string,
    type_number,
};

struct Typed_Value {
    enum Data_Type type;
    union {
        u8* string;
        u32 number;
    };
};

struct Token {
    enum Token_Type type;
    union {
        u8* str;
        u32 value;
        u8 character;
    };
    size start, end;
};

struct {
    size line, cursor;
    u8* program;
} interp;

struct Token interp_next_token() {
    struct Token token;
    token.character = 0;

    while (interp.program[interp.cursor] == ' ')
        interp.cursor++;
    token.start = interp.cursor;

    u8 c = interp.program[interp.cursor];
    if (c >= '0' && c <= '9') {
        size value = c - '0';
        c = interp.program[++interp.cursor];

        while (c >= '0' && c <= '9') {
            value *= 10;
            value += c - '0';
            c = interp.program[++interp.cursor];
        }
        token.end = interp.cursor;
        token.type = token_number;
        token.value = value;
        return token;
    }

    if (c >= 'a' && c <= 'z') {
        size length = 0;
        while (c >= 'a' && c <= 'z') {
            length++;
            c = interp.program[++interp.cursor];
        }
        u8* id = heap_allocate_typed(u8, length + 1);

        for (int n = 0; n < length; n++) {
            id[n] = interp.program[token.start + n];
        }
        id[length] = 0;

        token.end  = interp.cursor;
        token.type = token_name;
        token.str  = id;
        return token;
    }

    switch (c) {
        case '+':
        case '-':
        case '*':
        case '/':
            token.character = c;
            token.type = token_operator;
            break;
        case 0:
            token.type = token_line_end;
            break;
        default:
            token.type = token_error;
    }
    token.end = interp.cursor;
    interp.cursor++;
    return token;
}

struct Token interp_peek_token() {
    size old = interp.cursor;
    struct Token token = interp_next_token();
    interp.cursor = old;
    return token;
}

u32 interp_parse_product();
u32 interp_parse_factor();

bool expr_error;
u32 interp_parse_expression() {
    expr_error = false;
    u32 value = interp_parse_product();

    struct Token operator = interp_peek_token();
    while (operator.character == '+'
        || operator.character == '-') {

        operator = interp_next_token();
        switch (operator.character) {
            case '+': value += interp_parse_product(); break;
            case '-': value -= interp_parse_product(); break;
        }
        operator = interp_peek_token();
    }
    return value;
}

u32 interp_parse_product() {
    u32 value = interp_parse_factor();

    struct Token operator = interp_peek_token();
    while (operator.character == '*'
        || operator.character == '/') {

        operator = interp_next_token();
        switch (operator.character) {
            case '*': value *= interp_parse_factor(); break;
            case '/': value /= interp_parse_factor(); break;
        }
        operator = interp_peek_token();
    }
    return value;
}

u32 interp_parse_factor() {
    struct Token num = interp_next_token();
    if (num.type != token_number) {
        vga_print("\nExpected number, got token type: ");
        vga_print_hex(num.type);
        vga_newline();
        expr_error = true;
        return 0;
    }
    return num.value;
}

void interp_parse_command() {
    interp.line++;
    interp.cursor = 0;
    struct Token token = interp_peek_token();

    switch (token.type) {
    case token_line_end: return;
    case token_number: {
        u32 value = interp_parse_expression();
        if (expr_error) return;
        vga_newline();
        vga_print_hex(value);
        break;
    }
    case token_name: {
        vga_newline();
        vga_print(token.str);
        break;
    }
    default:
        vga_newline();
        vga_print_char('0' + token.type);
    }
}



struct {
    u8 command[vga_cols - 2];
    size length;
} shell;

void shell_submit_command() {
    interp.program = shell.command;
    interp_parse_command();

    shell.length = 0;
    vga_newline();
    vga_print("# ");
    vga_move_cursor(vga.cursor);
    for (size n = 0; n < sizeof(shell.command); n++)
        shell.command[n] = 0;
}

void shell_keypress(struct Kbd_Key key) {
    if (key.is_release) return;

    if (key.ascii) {
        if (shell.length > 65) return;
        if (key.ascii == 'c' && key.ctrl) {
            vga_print("^C");
            vga_newline();
            vga_print("# ");
            vga_move_cursor(vga.cursor);
            shell.length = 0;
            return;
        }

        vga_print_char(key.ascii);
        shell.command[shell.length++] = key.ascii;
        vga_move_cursor(vga.cursor);
    }

    else switch (key.scan_code) {
        case key_backspace:
            if (vga.cursor % vga_cols <= 2) break;
            vga.cursor--;
            vga_print_char(' ');
            vga.cursor--;
            shell.length--;
            shell.command[shell.length] = 0;
            vga_move_cursor(vga.cursor);
            break;
        case key_enter:
            shell_submit_command();
    }
    return;
}

void shell_init() {
    interp.line = 0;
    interp.cursor = 0;
    shell.length = 0;
    kbd_set_handler(&shell_keypress);

    vga_print("# ");
    vga_move_cursor(vga.cursor);
}

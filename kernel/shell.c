
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "kernel/interrupts.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "drivers/ata.h"

u8* msg_welcome =
"  /%%%%%%  /%%                           /%%  /%%%%%%   /%%%%%% \n"
" /%%__  %%| %%                          |__/ /%%__  %% /%%__  %%\n"
"| %%  \\ %%| %%   /%%  /%%%%%%   /%%%%%%  /%%| %%  \\ %%| %%  \\__/\n"
"| %%%%%%%%| %%  /%%/ /%%__  %% /%%__  %%| %%| %%  | %%|  %%%%%% \n"
"| %%__  %%| %%%%%%/ | %%%%%%%%| %%  \\__/| %%| %%  | %% \\____  %%\n"
"| %%  | %%| %%_  %% | %%_____/| %%      | %%| %%  | %% /%%  \\ %%\n"
"| %%  | %%| %% \\  %%|  %%%%%%%| %%      | %%|  %%%%%%/|  %%%%%%/\n"
"|__/  |__/|__/  \\__/ \\_______/|__/      |__/ \\______/  \\______/ \n\n";

u8* shell_title = "Akerios Shell";

internal void clear_screen() {
    vga_clear();
    vga_print("===[ ");
    vga_print(shell_title);
    vga_print(" ]");
    for (int n = 0; n < vga_cols - 7 - str_length(shell_title); n++) {
        vga_print_char('=');
    }
}


enum Data_Type {
    type_void,
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

typedef struct Typed_Value Argument_List[8];
typedef struct Typed_Value(*Shell_Command)(Argument_List);

internal bool shell_type_check_argument_list(Argument_List, Argument_List);

internal struct Typed_Value shell_cmd_set_colour(Argument_List args) {
    if (args[0].type) vga.attribute = args[0].number;

    struct Typed_Value ret;
    ret.type = type_number;
    ret.number = vga.attribute;
    return ret;
}

internal struct Typed_Value shell_cmd_clear(Argument_List args) {
    clear_screen();

    struct Typed_Value ret;
    ret.type = type_void;
    return ret;
}

internal struct Typed_Value shell_cmd_random(Argument_List args) {
    static struct Rand rand;
    rand.state = rdtsc().lower;

    struct Typed_Value ret;
    ret.type = type_number;

    if (args[0].type && args[1].type) ret.number = rand_range(&rand, args[0].number, args[1].number);
    else ret.number = rand_next_int(&rand); 
    return ret;
}

internal struct Typed_Value shell_cmd_alloc(Argument_List args) {
    struct Typed_Value ret;
    ret.type = type_number;
    if (args[0].type) ret.number = (u32) heap_allocate(args[0].number);
    return ret;
}

internal struct Typed_Value shell_cmd_free(Argument_List args) {
    struct Typed_Value ret;
    ret.type = type_void;
    if (args[0].type) heap_deallocate((void*) args[0].number);
    return ret;
}

internal struct Typed_Value shell_cmd_logo(Argument_List args) {
    vga_newline();
    vga_print(msg_welcome);
    struct Typed_Value ret;
    ret.type = type_void;
    return ret;
}

internal struct Typed_Value shell_cmd_hello(Argument_List args) {
    vga_newline();
    vga_print("Hello, World!");
    struct Typed_Value ret;
    ret.type = type_void;
    return ret;
}

internal struct Typed_Value shell_cmd_rdtsc(Argument_List args) {
    vga_newline();
    struct u64 tsc = rdtsc();
    vga_print("edx:");
    vga_print_hex(tsc.upper);
    vga_print("\neax:");
    vga_print_hex(tsc.lower);

    struct Typed_Value ret;
    ret.type = type_void;
    return ret;
}

internal struct Typed_Value shell_cmd_poke(Argument_List args) {
    size size = 1;
    if (args[2].type == type_number) size = args[2].number;
    void* target = cast(void*, args[0].number);
    u32 value    =             args[1].number;

    struct Typed_Value ret;
    if (args[1].type) {
        switch (size) {
            case 1: *cast(u8*,  target) = value; break;
            case 2: *cast(u16*, target) = value; break;
            case 4: *cast(u32*, target) = value; break;
            default: vga_print("Poke invalid size");
        }
        ret.type = type_void;
    }
    else {
        ret.type = type_number;
        ret.number = *cast(u16*, target);
    }
    return ret;
}

internal struct Typed_Value shell_cmd_read(Argument_List args) {
    void* buffer = cast(void*, args[0].number);
    u32 sector = args[1].number;
    ata_lba_read(buffer, sector, args[2].type ? args[2].number : 1);

    struct Typed_Value ret;
    ret.type = type_void;
    return ret;
}

Shell_Command shell_command_funcs[10] = {
    &shell_cmd_set_colour,
    &shell_cmd_random,
    &shell_cmd_clear,
    &shell_cmd_alloc,
    &shell_cmd_free,
    &shell_cmd_logo,
    &shell_cmd_hello,
    &shell_cmd_rdtsc,
    &shell_cmd_poke,
    &shell_cmd_read,
};

i8* shell_command_strings[10] = {
    "colour", "random", "clear", "allocate", "deallocate", "akerios", "hello", "rdtsc", "poke", "disk:read"
};

enum Token_Type {
    token_name,
    token_number,
    token_string,
    token_operator,
    token_comma,
    token_line_end,
    token_error,
    token_open_bracket,
    token_close_bracket,
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

internal struct Token interp_next_token() {
    struct Token token;
    token.character = 0;

    while (interp.program[interp.cursor] == ' ')
        interp.cursor++;
    token.start = interp.cursor;

    u8 c = interp.program[interp.cursor];
    if (c >= '0' && c <= '9') {
        size value = c - '0';

        bool hex = false;
        if (c == '0' && interp.program[interp.cursor+1] == 'x') {
            hex = true;
            interp.cursor++;
        }
        c = interp.program[++interp.cursor];

        if (!hex) while (c >= '0' && c <= '9') {
            value *= 10;
            value += c - '0';
            c = interp.program[++interp.cursor];
        }

        else while (c >= '0' && c <= '9' || c >= 'a' && c <= 'f') {
            value *= 16;
            if (c > '9') value += c - 'f' + 15;
            else value += c - '0';
            c = interp.program[++interp.cursor];
        }

        token.end = interp.cursor;
        token.type = token_number;
        token.value = value;
        return token;
    }

    if (c >= 'a' && c <= 'z') {
        size length = 0;
        while (c >= 'a' && c <= 'z' || c == ':') {
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
        case 0:   token.type = token_line_end;      break;
        case '[': token.type = token_open_bracket;  break;
        case ']': token.type = token_close_bracket; break;
        case ',': token.type = token_comma;         break;
        default:
            token.type = token_error;
    }
    token.end = interp.cursor;
    interp.cursor++;
    return token;
}

internal struct Token interp_peek_token() {
    size old = interp.cursor;
    struct Token token = interp_next_token();
    interp.cursor = old;
    return token;
}

internal u32 interp_parse_product();
internal u32 interp_parse_factor();

bool expr_error;
internal u32 interp_parse_expression() {
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

internal u32 interp_parse_product() {
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

internal struct Typed_Value* interp_parse_argument_list() {
    struct Typed_Value* list = heap_allocate(size_of(struct Typed_Value) * 8);
    mem_clear(cast(void*, list), size_of(struct Typed_Value) * 8);

    if (interp_peek_token().type == token_open_bracket) {
        size n = 0;
        do {
            interp_next_token();
            list[n].type = type_number;
            list[n++].number = interp_parse_expression();
        } while (interp_peek_token().type == token_comma);
        if (interp_next_token().type != token_close_bracket) {
            vga_print("\nExpected closed bracket");
            expr_error = true;
        }
    }
    return list;
}

internal u32 interp_parse_factor() {
    struct Token factor = interp_next_token();
    switch (factor.type) {
        case token_number: return factor.value;
        case token_name: {
            for (size n = 0; n < size_of(shell_command_strings) / size_of(i8*); n++) {
                if (str_compare(factor.str, shell_command_strings[n])) {
                    struct Typed_Value* list = interp_parse_argument_list();
                    struct Typed_Value value = shell_command_funcs[n](list);
                    return value.number;
                }
            }
        }
        default:
            vga_newline();
            vga_print("ERROR");
    }
    return factor.value;
}

void interp_parse_command() {
    interp.line++;
    interp.cursor = 0;
    struct Token token = interp_peek_token();

    switch (token.type) {
    case token_line_end: return;
    case token_name:
    case token_number: {
        u32 value = interp_parse_expression();
        if (expr_error) return;
        vga_newline();
        vga_print_hex(value);
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
    u8 prompt;
} shell;

internal void shell_scroll() {
    size row = 2 * vga_cols;
    mem_move(vga.framebuffer + row,
             vga.framebuffer + row + row,
             (vga_rows) * row);

    mem_set_typed(u16, vga.framebuffer + (vga_rows-1) * row, vga_cols, vga.attribute << 8);
    vga.cursor -= vga_cols;
}

internal void shell_submit_command() {
    interp.program = shell.command;
    interp_parse_command();

    shell.length = 0;
    vga_newline();

    vga_move_cursor(vga.cursor);
    vga_print_char(shell.prompt);
    vga_print_char(' ');
    vga_move_cursor(vga.cursor);

    for (size n = 0; n < sizeof(shell.command); n++) {
        shell.command[n] = 0;
    }
}

internal void shell_keypress(struct Kbd_Key key) {
    if (key.is_release) return;

    if (key.ascii) {
        if (shell.length > 65) return;
        if (key.ascii == 'c' && key.ctrl) {
            vga_print("^C");
            vga_newline();
            vga_print_char(shell.prompt);
            vga_print_char(' ');
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
    shell.prompt = 0xe4;
    kbd_set_handler(&shell_keypress);
    clear_screen();

    vga_print_char(shell.prompt);
    vga_print_char(' ');
    vga_move_cursor(vga.cursor);
    vga_set_spill_handler(&shell_scroll);
}

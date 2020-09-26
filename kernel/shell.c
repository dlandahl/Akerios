
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "kernel/interrupts.h"
#include "kernel/filesystem.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "drivers/ata.h"

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

struct {
    u8 command[vga_cols - 2];
    size length;
    u8 prompt;
    u8* title;

    Shell_Command* command_funcs;
    i8** command_strings;
    size command_count;
} shell;

internal void clear_screen() {
    vga_clear();

    vga_print("===[ ");
    vga_print(shell.title);
    vga_print(" ]");
    for (int n = 0; n < vga_cols - 7 - str_length(shell.title); n++) {
        vga_print_char('=');
    }
}

internal bool shell_type_check_argument_list(Argument_List, Argument_List);

#define RETURN_NOTHING       \
    struct Typed_Value ret;  \
    ret.type = type_void;    \
    return ret               \

internal struct Typed_Value shell_cmd_set_colour(Argument_List args) {
    if (args[0].type) vga.attribute = args[0].number;

    struct Typed_Value ret;
    ret.type = type_number;
    ret.number = vga.attribute;
    return ret;
}

internal struct Typed_Value shell_cmd_clear(Argument_List args) {
    clear_screen();
    RETURN_NOTHING;
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
    if (args[0].type) heap_deallocate((void*) args[0].number);
    RETURN_NOTHING;
}

internal struct Typed_Value shell_cmd_logo(Argument_List args) {
    u8* msg_welcome =
"\n  /%%%%%%  /%%                           /%%  /%%%%%%   /%%%%%% \n"
" /%%__  %%| %%                          |__/ /%%__  %% /%%__  %%\n"
"| %%  \\ %%| %%   /%%  /%%%%%%   /%%%%%%  /%%| %%  \\ %%| %%  \\__/\n"
"| %%%%%%%%| %%  /%%/ /%%__  %% /%%__  %%| %%| %%  | %%|  %%%%%% \n"
"| %%__  %%| %%%%%%/ | %%%%%%%%| %%  \\__/| %%| %%  | %% \\____  %%\n"
"| %%  | %%| %%_  %% | %%_____/| %%      | %%| %%  | %% /%%  \\ %%\n"
"| %%  | %%| %% \\  %%|  %%%%%%%| %%      | %%|  %%%%%%/|  %%%%%%/\n"
"|__/  |__/|__/  \\__/ \\_______/|__/      |__/ \\______/  \\______/";
    struct Typed_Value ret;
    ret.type = type_string;
    ret.string = msg_welcome;
    return ret;
}

internal struct Typed_Value shell_cmd_hello(Argument_List args) {
    struct Typed_Value ret;
    ret.type = type_string;
    ret.string = "Hello World";
    return ret;
}

internal struct Typed_Value shell_cmd_rdtsc(Argument_List args) {
    vga_newline();
    struct u64 tsc = rdtsc();
    vga_print("edx:");
    vga_print_hex(tsc.upper);
    vga_print("\neax:");
    vga_print_hex(tsc.lower);

    RETURN_NOTHING;
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

    RETURN_NOTHING;
}

internal struct Typed_Value shell_cmd_highlight(Argument_List args) {
    i8* message = args[0].string;
    u32 colour = vga_red;
    if (args[1].type) colour = args[1].number;
    u8 attr = vga.attribute;

    u8 tag[3] = "\\c";
    size size = str_length(message);
    i8* buffer = heap_allocate(size + size_of(tag) * 2);

    tag[2] = (vga.attribute & 0xf0) + (colour & 0xf);
    mem_copy(buffer, tag, size_of(tag));
    mem_copy(buffer + size_of(tag), message, size);
    tag[2] = vga.attribute;
    mem_copy(buffer + size_of(tag) + size, tag, size_of(tag));

    struct Typed_Value ret;
    ret.type = type_string;
    ret.string = buffer;
    return ret;
}

internal struct Typed_Value shell_cmd_fs_format(Argument_List args) {
    fs_format();
    RETURN_NOTHING;
}

internal struct Typed_Value shell_cmd_fs_append(Argument_List args) {
    u8* file = args[0].string;
    u8* data = args[1].string;
    size count = str_length(data);
    fs_append_to_file(file, data, count);
    RETURN_NOTHING;
}

internal struct Typed_Value shell_cmd_fs_create(Argument_List args) {
    fs_create_file(args[0].string);
    RETURN_NOTHING;
}

internal struct Typed_Value shell_cmd_fs_list(Argument_List args) {
    vga_newline();
    fs_list_directory();
    RETURN_NOTHING;
}

internal struct Typed_Value shell_cmd_fs_write(Argument_List args) {
    u8* file = args[0].string;
    u8* data = args[1].string;
    size count = str_length(data);
    fs_write_entire_file(file, data, count);
    RETURN_NOTHING;
}

internal struct Typed_Value shell_cmd_fs_read(Argument_List args) {
    u8* file = args[0].string;

    struct Typed_Value ret;
    ret.type = type_string;
    ret.string = fs_read_entire_file(file);
    return ret;
}

internal struct Typed_Value shell_cmd_fs_commit(Argument_List args) {
    fs_commit();
    vga_print("\nCommited");
    RETURN_NOTHING;
}

internal struct Typed_Value shell_cmd_strtok(Argument_List args) {
    u8* ret;
    if (args[0].type == type_string) ret = str_tokenize(args[0].string, '|');
    else ret = str_tokenize(nullptr, '|');

    struct Typed_Value value;
    value.string = ret;
    value.type = type_string;
    return value;
}

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
    bool error_flag;
} interp;



internal inline bool is_alpha(i8 c) {
    return c >= 'a' && c <= 'z';
}

internal inline bool is_num(i8 c) {
    return c >= '0' && c <= '9';
}

internal inline bool is_hex(i8 c) {
    return (c >= '0' && c <= '9' || c >= 'a' && c <= 'f');
}



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

        if (!hex) while (is_num(c)) {
            value *= 10;
            value += c - '0';
            c = interp.program[++interp.cursor];
        }

        else while (is_hex(c)) {
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

    if (is_alpha(c)) {
        size length = 0;
        while (is_alpha(c) || c == ':') {
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
        case '"': {
            c = interp.program[++interp.cursor];
            i8* string = heap_allocate(128);
            for (size n = 0; c != '"'; n++) {
                string[n] = c;
                c = interp.program[++interp.cursor];
            }
            token.type = token_string;
            token.str = string;
            break;
        }
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

internal struct Typed_Value interp_parse_product();
internal struct Typed_Value interp_parse_factor();

bool expr_error;
internal struct Typed_Value interp_parse_expression() {
    expr_error = false;
    struct Typed_Value value = interp_parse_product();
    if (value.type != type_number) return value;

    struct Token operator = interp_peek_token();
    while (operator.character == '+'
        || operator.character == '-') {

        operator = interp_next_token();
        switch (operator.character) {
            case '+': value.number += interp_parse_product().number; break;
            case '-': value.number -= interp_parse_product().number; break;
        }
        operator = interp_peek_token();
    }
    return value;
}

internal struct Typed_Value interp_parse_product() {
    struct Typed_Value value = interp_parse_factor();
    if (value.type != type_number) return value;

    struct Token operator = interp_peek_token();
    while (operator.character == '*'
        || operator.character == '/') {

        operator = interp_next_token();
        switch (operator.character) {
            case '*': value.number *= interp_parse_factor().number; break;
            case '/': value.number /= interp_parse_factor().number; break;
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
            list[n++] = interp_parse_expression();
        } while (interp_peek_token().type == token_comma);
        if (interp_next_token().type != token_close_bracket) {
            vga_print("\nExpected closed bracket");
            expr_error = true;
        }
    }
    return list;
}

internal struct Typed_Value interp_parse_factor() {
    struct Token factor = interp_next_token();
    struct Typed_Value value;

    switch (factor.type) {
        case token_number: {
            value.type = type_number;
            value.number = factor.value;
            return value;
        }
        case token_name: {
            for (size n = 0; n < 19; n++) {
                if (str_compare(factor.str, shell.command_strings[n])) {
                    struct Typed_Value* list = interp_parse_argument_list();
                    return shell.command_funcs[n](list);
                }
            }
        }
        case token_string: {
            value.type = type_string;
            value.string = factor.str;
            return value;
        }
        default:
            vga_newline();
            vga_print("ERROR");
    }
    return value;
}

void interp_parse_command() {
    interp.line++;
    interp.cursor = 0;
    struct Token token = interp_peek_token();

    switch (token.type) {
    case token_line_end: return;
    case token_name:
    case token_string:
    case token_number: {
        struct Typed_Value value = interp_parse_expression();
        if (expr_error) return;
        switch (value.type) {
            case type_number: vga_newline(); vga_print_hex(value.number); break;
            case type_string: vga_newline(); vga_print(value.string); break;
            case type_void: break;
        }
        break;
    }
    default:
        vga_newline();
        vga_print_char('0' + token.type);
    }
}



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

    Shell_Command commands[] = {
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
        &shell_cmd_highlight,
        &shell_cmd_fs_format,
        &shell_cmd_fs_append,
        &shell_cmd_fs_create,
        &shell_cmd_fs_list,
        &shell_cmd_fs_write,
        &shell_cmd_fs_read,
        &shell_cmd_fs_commit,
        &shell_cmd_strtok,
    };

    i8* strings[] = {
        "colour", "random", "clear", "alloc", "free", "akerios", "hello", "rdtsc", "poke", "read", "highlight",
        "fs:format", "fs:append", "fs:create", "fs:list", "fs:write", "fs:read", "fs:commit",
        "str:tok"
    };

    shell.command_strings = heap_allocate(size_of(strings));
    mem_copy(shell.command_strings, strings, size_of(strings));

    shell.command_funcs = heap_allocate(size_of(commands));
    mem_copy(shell.command_funcs, commands, size_of(commands));

    shell.command_count = size_of(commands) / size_of(Shell_Command);
    shell.title = "Akerios Shell";
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

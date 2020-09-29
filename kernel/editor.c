
#include "common.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"

struct Akerios_Mode editor_mode;
struct {
    u8* buffer;
} editor;

void editor_keypress(struct Kbd_Key* key) {

}

void editor_init() {
    editor_mode.title = "Editor";
    editor_mode.key_handler = &editor_keypress;
    editor.buffer = heap_allocate(1);
}


%define vbs 0x10
%define vbs_tty      0x0e
%define vbs_setpixel 0x0c
%define vbs_mode_graphic4 0x12
%define vbs_mode_text16   0x03

%define vga_address 0xb8000
%define vga_col_wb  0x0f
%define vga_col_gb  0x0a
%define vga_col_rw  0xf4

%define kbs 0x16
%define kbs_read 0x00

%define dbs 0x13
%define dbs_read_sector 0x02

%define MARKER(name) name equ $ - 0x7c00

%define kernel_offset 0x1000
[org 0x7c00]

[bits 16]
    xor ax, ax
    mov es, ax
    mov ds, ax

    mov [boot_drive], dl

    mov bp, 0x8000
    mov sp, bp
    mov ss, bp

    mov bx, bootsector_size
    call print_hex

    mov bx, msg_welcome
    call print_ln

    call load_kernel

    call enable_protmode
    jmp $

load_kernel:
    mov bx, msg_load
    call print_ln

    mov bx, kernel_offset
    mov dh, 0x19
    mov dl, [boot_drive]

    call read_sectors
    ret

%include "gdt.asm"
%include "bootsector_lib_real.asm"
%include "bootsector_lib_prot.asm"

protmode_begin:
    ; call clear_screen

    mov [print.colour], byte vga_col_gb
    mov ebx, msg_protmode
    call print

    call kernel_offset

    jmp $

msg_welcome:  db 'Welcome to the operating system', 0
msg_load:     db 'Loading kernel', 0
msg_protmode: db 'Entered x86 protected mode', 0
boot_drive:   db 0

MARKER(bootsector_size)

times 510-($-$$) db 0
dw 0xaa55

MARKER(program_size)

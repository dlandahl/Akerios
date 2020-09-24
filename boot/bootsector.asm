
%define vbs 0x10
%define vbs_tty 0x0e

%define vga_address 0xb8000
%define vga_col_wb  0x0f
%define vga_col_gb  0x0a

%define kbs 0x16
%define kbs_read 0x00

%define dbs 0x13
%define dbs_read_sector 0x02

%define MARKER(name) name equ $ - 0x7c00
%define kernel_offset 0x1000

[org 0x7c00]

[bits 16]
    jmp 0:start
start:
    xor ax, ax
    mov es, ax
    mov ds, ax
    mov ss, ax

    mov [boot_drive], dl

    mov bp, 0x7c00
    mov sp, bp

    mov bx, bootsector_size
    call print_hex

    call dummy

    mov bx, msg_welcome
    call real_print

    call load_kernel

    call enable_protmode
    jmp $

load_kernel:
    mov bx, msg_load
    call print_ln

    mov bx, kernel_offset
    mov dh, 0x35
    mov dl, [boot_drive]
    call read_sectors

    mov bx, ax
    call print_hex

    mov bx, msg_loaded
    call print_ln
    ret

dummy:
    ret

%include "./boot/gdt.asm"
%include "./boot/bootsector_lib_real.asm"
%include "./boot/bootsector_lib_prot.asm"

protmode_begin:

    mov [print.colour], byte vga_col_gb
    mov ebx, msg_protmode
    call print

    call kernel_offset


msg_welcome:  db 'Welcome to the OS', 0
msg_load:     db 'Loading kernel', 0
msg_protmode: db 'Entered protected mode', 0
msg_loaded:   db 'Loaded kernel', 0
boot_drive:   db 0

MARKER(bootsector_size)

times 0x1fe-($-$$) db 0
dw 0xaa55

MARKER(program_size)

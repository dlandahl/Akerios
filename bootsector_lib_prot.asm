
[bits 16]
enable_protmode:
    mov bx, .msg
    call print_ln

    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or  eax, 1 << 0
    mov cr0, eax
    jmp codeseg:protmode.init

.msg: db 'Enabling protected mode', 0



[bits 32]
protmode:
.init:
    mov ax, dataseg
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000
    mov esp, ebp

    call protmode_begin


%define row_size 160

print:
    pusha

.get_position:
    xor eax, eax
    mov ax, word[.rows]
    mov edx, row_size
    mul edx
    add eax, vga_address
    mov edx, eax

.loop:
    mov al, [ebx]
    mov ah, [.colour]

    cmp al, 0
    je .end

    inc ebx
    cmp al, 10
    je .newline

    mov [edx], ax
    add edx, 2
    jmp .loop

.end:
    inc word[.rows]
    popa
    ret

.newline:
    inc word[.rows]
    jmp .get_position

.colour: db vga_col_wb
.rows: dw 0

; clear_screen:
;     pusha
;     mov eax, 0xffff
; 
; .loop:
;     dec eax
;     cmp eax, 0
;     je .end
;     mov word[vga_address + eax], 0
;     jmp .loop
; 
; .end:
;     popa
;     ret

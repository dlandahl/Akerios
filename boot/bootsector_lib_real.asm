
real_print:
    mov ah, vbs_tty

.loop:
    mov al, [bx]
    cmp al, 0
    je .end
    inc bx
    int vbs

    jmp .loop

.end:
    ret



print_ln:
    call real_print
    mov bx, lfcr
    call real_print
    ret

lfcr: db 0xa, 0xd, 0


; read_ch:
;     xor ax, ax
;     mov ah, kbs_read
;     int kbs
; 
;     and ax, 0x00ff
;     mov [.char], ax
;     mov bx, .char
;     call print
;     mov ax, [.char]
; 
;     ret
; 
; .char: db 0


; sleep:
;     push ax
;     mov ah, 0x86
;     int 0x15
;     pop ax
;     ret


print_hex:
    mov cx, bx
    mov bx, .prefix
    call real_print
    mov bx, cx
    mov cx, 12

.loop:
    push bx
    sar bx, cl
    and bx, 0xf
    mov ax, [.hexdigits + bx]
    mov [.char], al
    mov bx, .char
    call real_print

    pop bx
    sub cx, 4
    cmp cx, 0
    jge .loop

    mov bx, lfcr
    call real_print
    ret

.hexdigits: db '0123456789abcdef', 0
.char: db 0, 0
.prefix: db '0x', 0



read_sectors:
    push dx

    mov ah, dbs_read_sector
    mov al, dh
    mov ch, 0
    mov dh, 0
    mov cl, 2

    int dbs
    pop dx
    jc .error

    cmp dh, al
    jne .error

    ret
.error:
    mov bx, ax
    call print_hex
    mov bx, .msg_error
    call print_ln
    ret
    ; jmp $

.msg_error: db 'Failed to load some sectors from disk', 10, 0



; %define screen_width  300
; %define screen_height 300
; 
; setpixel:
;     xor ah, ah
;     mov al, vbs_mode_graphic4
;     int vbs
; 
;     mov ah, vbs_setpixel
;     xor bh, bh
;     xor cx, cx
;     xor dx, dx
; 
; .outer:
;     inc dx
;     xor cx, cx
;     cmp dx, screen_height
;     je .end
; 
; .inner:
;     int vbs
;     inc cx
; 
;     ; push cx
;     ; mov cx, 1
;     ; call sleep
;     ; pop cx
; 
;     inc al
;     cmp cx, screen_width
;     jge .outer
;     jmp .inner
; 
; .end:
;     ret
; 

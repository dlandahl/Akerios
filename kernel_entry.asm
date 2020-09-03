
[bits 32]
[extern entry_point]
    mov eax, 0xfeedeef
    call entry_point
    jmp $


[bits 32]
[extern kernel_entry]
    mov eax, 0xfeedeef
    call kernel_entry
    jmp $

; 
; gdt_limit_code equ 0xffffff
; gdt_base_code  equ 0x0
; 
; ; 1st flags:
; gdtf_present             equ 1 << 3
; gdtf_type_data_or_code   equ 1 << 0
; 
; ; Type flags:
; gdtf_code                equ 1 << 3
; gdtf_conforming          equ 1 << 2
; gdtf_readable            equ 1 << 1
; gdtf_accessed            equ 1 << 0
; 
; ; 2nd flags:
; granularity              equ 1 << 3
; bits32                   equ 1 << 2
; avl                      equ 1 << 0
; 
; gdt_first_flags equ gdtf_present | gdtf_type_data_or_code
; gdt_type_flags equ gdtf_code | gdtf_readable
; 

gdt:
.start:
.null_descriptor:
    dd 0x0
    dd 0x0

.segment_code:
    dw 0xffff
    dw 0x0
    db 0x0
    db 10011010b
    db 11001111b
    db 0x0

.segment_data:
    dw 0xffff
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0

.end:


gdt_desc:
    dw gdt.end - gdt.start - 1
    dd gdt.start

codeseg equ gdt.segment_code - gdt.start
dataseg equ gdt.segment_data - gdt.start

%define GDT_SEGMENT(n) gdt + n * 8
%define GDT_CODE 1
%define GDT_DATA 2

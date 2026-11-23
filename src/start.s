.section .text.start,"ax"
.code32
.global _start
.extern kmain
.extern __bss_start
.extern __bss_end
_start:
    cld
    xorl %eax, %eax
    movl $__bss_start, %edi
    movl $__bss_end, %ecx
    subl %edi, %ecx
    rep stosb
    call kmain
halt_forever:
    cli
    hlt
    jmp halt_forever
.section .note.GNU-stack,"",@progbits

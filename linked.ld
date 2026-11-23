OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(_start)
PHDRS
{
    text PT_LOAD FLAGS(5); /* read + execute */
    data PT_LOAD FLAGS(6); /* read + write */
}
SECTIONS
{
    . = 0x10000;
    .text : { *(.text.start) *(.text*) } :text
    . = ALIGN(4096);
    .user_text : { *(.user_text) } :text
    . = ALIGN(4096);
    .rodata : { *(.rodata*) } :text
    . = ALIGN(4096);
    .data : { *(.data*) } :data
    .bss (NOLOAD) : { __bss_start = .; *(COMMON) *(.bss*) __bss_end = .; } :data
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}

/* Native IA-32 port of the original INILOA separately loaded image. */
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(port_iniloa_entry)
PHDRS { text PT_LOAD FLAGS(5); data PT_LOAD FLAGS(6); }
SECTIONS {
  . = 0x00380000;
  .text : { *(.text*) *(.rodata*) } :text
  . = ALIGN(4096);
  .data : { *(.data*) } :data
  . = ALIGN(4096);
  .bss (NOLOAD) : { *(COMMON) *(.bss*) } :data
  /DISCARD/ : { *(.note*) *(.comment*) *(.eh_frame*) }
  ASSERT(. <= 0x003e0000, "INILOA exceeds reserved supervisor aperture")
}

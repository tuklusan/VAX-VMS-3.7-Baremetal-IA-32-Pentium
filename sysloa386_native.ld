/* SYSLOAxxx.EXE successor: one native IA-32 CPU-dependent loadable module.
 * All entry points remain in supervisor RAM, outside SYS/INILOA and the
 * bootstrap. The original self-relative vector header precedes code. */
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(port_sysloa386_entry)
PHDRS { text PT_LOAD FLAGS(5); data PT_LOAD FLAGS(6); }
SECTIONS {
  . = 0x003e0000;
  .text : { KEEP(*(.vmsvec)) *(.text*) *(.rodata*) } :text
  . = ALIGN(4096);
  .data : { *(.data*) } :data
  . = ALIGN(4096);
  .bss (NOLOAD) : { *(COMMON) *(.bss*) } :data
  . = ALIGN(16);
  port_sysloa386_image_end = .;
  /DISCARD/ : { *(.note*) *(.comment*) *(.eh_frame*) }
  ASSERT(. <= 0x003f0000, "SYSLOA386 exceeds reserved supervisor aperture")
  ASSERT(port_sysloa386_image_end - port_sysloa386_descriptor <= 65535,
         "SYSLOA386 self-relative vector table exceeds 16-bit header")
}

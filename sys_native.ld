/* Native SYS.EXE: a separately loaded IA-32 executive, never a kernel stub.
 * 0x320000..0x380000 is excluded from the 1..3MiB page allocator. */
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(port_sys_executive_entry)
PHDRS { text PT_LOAD FLAGS(5); data PT_LOAD FLAGS(6); }
SECTIONS {
  . = 0x00320000;
  .text : { *(.text*) *(.rodata*) } :text
  . = ALIGN(4096);
  .data : { *(.data*) *(.vms_pfn_fixup*) } :data
  . = ALIGN(4096);
  .vms_sys_end : { *(.vms_sys_end*) } :data
  . = ALIGN(4096);
  .bss (NOLOAD) : { *(COMMON) *(.bss*) } :data
  /DISCARD/ : { *(.note*) *(.comment*) *(.eh_frame*) }
  ASSERT(. <= 0x00380000, "SYS.EXE exceeds the reserved supervisor aperture")
}

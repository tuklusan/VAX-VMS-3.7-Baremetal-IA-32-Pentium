#ifndef VMS_SYSBOOT_ELF_H
#define VMS_SYSBOOT_ELF_H
#include <stdint.h>
#include "vms_bootio.h"
/* SYSBOOT LOAD_CODE for the native IA-32 SYS.EXE.  VAX image records are
 * machine-specific; ELF32 PT_LOAD replaces their storage description, not
 * the Files-11 file lookup, read, allocation, or initialization sequence.
 * The destination is a reserved supervisor-only identity-mapped aperture. */
enum {
    PORT_SYS_ELF_START=0x00320000u,
    PORT_SYS_ELF_LIMIT=0x00380000u,
    PORT_INILOA_ELF_START=0x00380000u,
    PORT_INILOA_ELF_LIMIT=0x003e0000u,
    PORT_SYSLOA386_ELF_START=0x003e0000u,
    PORT_SYSLOA386_ELF_LIMIT=0x003f0000u,
    PORT_SYS_ELF_OK=1u,
    PORT_SYS_ELF_BAD_ARG=2u,
    PORT_SYS_ELF_BAD_HEADER=4u,
    PORT_SYS_ELF_BAD_SEGMENT=6u,
    PORT_SYS_ELF_DISK_ERROR=8u,
    PORT_SYS_ELF_WRITE_ERROR=10u
};
struct port_sys_elf_memory {
    void *context;
    int (*write)(void *,uint32_t,const uint8_t *,uint32_t);
    int (*clear)(void *,uint32_t,uint32_t);
};
uint32_t port_sysboot_load_elf(const struct boo_io_driver *disk,
                              uint32_t read_function,
                              const uint8_t file_header[512],
                              uint32_t file_blocks,
                              const struct port_sys_elf_memory *memory,
                              uint32_t *entry_out);
/* The second SYSBOOT LOAD_CODE pass maps INILOA in its own aperture.
 * This is independent of the previous SYS.EXE load and validates all
 * file contents through the same indexed Files-11 device driver. */
uint32_t port_sysboot_load_elf_range(const struct boo_io_driver *disk,
                              uint32_t read_function,
                              const uint8_t file_header[512],
                              uint32_t file_blocks,
                              const struct port_sys_elf_memory *memory,
                              uint32_t first_address,uint32_t limit_address,
                              uint32_t *entry_out);
#endif

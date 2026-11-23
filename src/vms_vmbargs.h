#ifndef VMS_VMBARGS_H
#define VMS_VMBARGS_H
#include <stddef.h>
#include <stdint.h>
#include "vms_bootio.h"

/* BOOTS/SRC/BOOTS.MAR: $VMBARGDEF, including the leading argument count.
 * Physical page numbers retain the original 512-byte page size. */
struct port_vmb_arguments {
    uint32_t count;
    uint32_t cache_bytes, cache_address;
    uint32_t lowest_pfn, highest_pfn;
    uint32_t bitmap_bytes, bitmap_address;
    uint32_t microcode_bytes, microcode_address;
    uint8_t scs_system_id[6];
    uint16_t spare;
    uint32_t flags, ci_highest_pfn;
};
_Static_assert(sizeof(struct port_vmb_arguments)==52u,"VMBARGDEF length");
_Static_assert(offsetof(struct port_vmb_arguments,cache_bytes)==4u,"VMB FILECACHE");
_Static_assert(offsetof(struct port_vmb_arguments,lowest_pfn)==12u,"VMB LO PFN");
_Static_assert(offsetof(struct port_vmb_arguments,highest_pfn)==16u,"VMB HI PFN");
_Static_assert(offsetof(struct port_vmb_arguments,bitmap_bytes)==20u,"VMB PFNMAP");
_Static_assert(offsetof(struct port_vmb_arguments,microcode_bytes)==28u,"VMB UCODE");
_Static_assert(offsetof(struct port_vmb_arguments,scs_system_id)==36u,"VMB SYSTEMID");
_Static_assert(offsetof(struct port_vmb_arguments,flags)==44u,"VMB FLAGS");
_Static_assert(offsetof(struct port_vmb_arguments,ci_highest_pfn)==48u,"VMB CI PFN");

enum {PORT_VMB_FLAG_LOAD_SCS=1u, PORT_VMB_E820_BASE=0x5000u,
      PORT_VMB_E820_COUNT=0x4ff0u, PORT_VMB_E820_LIMIT=32u,
      PORT_VMB_MAP_BYTES=262144u, PORT_VMB_PAGE_SIZE=512u};
struct port_pc_memory_range {
    uint32_t base_lo, base_hi, size_lo, size_hi, type;
};
_Static_assert(sizeof(struct port_pc_memory_range)==20u,"PC E820 entry");

/* An unsuccessful probe must not turn into guessed usable memory. */
enum {PORT_VMB_OK=1u,PORT_VMB_BADARGS=2u,PORT_VMB_NO_RAM=4u,
      PORT_VMB_MAP_OVERFLOW=6u};
uint32_t port_vmb_memory_arguments(struct port_vmb_arguments *args,
                                  uint8_t *pfn_bitmap,uint32_t capacity,
                                  const struct port_pc_memory_range *ranges,
                                  uint32_t count,uint32_t reserved_end);

/* BOOTS.MAR $BOOT_DRIVER record. Relative offsets are byte offsets
 * from the table entry, except ENTRY and DRIVRNAME, which use ADDR. */
struct port_boot_driver_record {
    int16_t cpu_type, device_type;
    int32_t action_offset;
    uint32_t driver_size;
    int32_t driver_offset, entry_offset, name_offset;
    int32_t aux_name_offset, unit_init_offset;
};
_Static_assert(sizeof(struct port_boot_driver_record)==32u,"BOOT DRIVER record");
struct port_boot_driver_registration {
    struct port_boot_driver_record record;
    struct boo_io_driver io;
    uint8_t name[5];
};
struct port_boot_driver_specs {
    int16_t cpu_type, device_type;
    const void *action, *driver, *entry, *name, *aux_name, *unit_init;
    uint32_t driver_bytes;
};
uint32_t port_boot_driver_encode(struct port_boot_driver_record *out,
                                 const struct port_boot_driver_specs *specs);
uint32_t port_boot_driver_register(struct port_boot_driver_registration *out,
                               const struct boo_io_driver *io,
                               int16_t cpu_type,int16_t device_type);
uint32_t port_boot_driver_select(const struct port_boot_driver_registration *table,
                                 size_t count,int16_t cpu_type,int16_t device_type,
                                 const struct boo_io_driver **driver);
#endif

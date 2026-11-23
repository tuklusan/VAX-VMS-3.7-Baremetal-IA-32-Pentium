#ifndef VMS_EXE_IMAGE_END85_H
#define VMS_EXE_IMAGE_END85_H
#include <stdint.h>
#include <stddef.h>

/* DEBUGDATA.MAR and MDATEND.MAR data used by the native SYS image. */
enum { PORT_BUG_STACK_WORDS=94, PORT_BUG_PRREG_WORDS=16,
       PORT_BUG_SAVR_WORDS=14, PORT_BUG_WORDS=128,
       PORT_PFN_PATCH_ENTRIES=16, PORT_PFN_PATCH_ENTRY_BYTES=6 };
struct port_bugcheck_debugdata {
    uint32_t stack[PORT_BUG_STACK_WORDS];
    uint32_t cpu_registers[PORT_BUG_PRREG_WORDS];
    uint32_t general_registers[PORT_BUG_SAVR_WORDS];
    uint32_t stack_pointer;
    uint32_t return_address;
    uint32_t processor_status;
    uint32_t flag;
};
extern struct port_bugcheck_debugdata port_bug_debug;

struct port_pfn_patch {
    uint32_t address;
    uint8_t old_opcode,new_opcode;
} __attribute__((packed));
struct port_pfn_patch_table {
    struct port_pfn_patch entries[PORT_PFN_PATCH_ENTRIES];
    uint32_t terminator;
};
struct port_pfn_patch_descriptor {
    uint32_t capacity;
    struct port_pfn_patch *address;
};
extern struct port_pfn_patch_table port_mmg_pfn_patch;
extern struct port_pfn_patch_descriptor port_mmg_pfn_patch_desc;
extern const uint8_t port_mmg_sys_end[];
#endif

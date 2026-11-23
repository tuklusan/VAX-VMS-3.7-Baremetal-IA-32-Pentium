#include "vms_exe_image_end85.h"

/* DEBUGDATA.MAR: 512-byte page-aligned bugcheck and panic register store. */
struct port_bugcheck_debugdata port_bug_debug __attribute__((aligned(4096)));
_Static_assert(sizeof(struct port_bugcheck_debugdata)==PORT_BUG_WORDS*4u,
               "Original DEBUGDATA.MAR occupies one 512-byte VMS page");
_Static_assert(offsetof(struct port_bugcheck_debugdata,cpu_registers)==94u*4u,
               "Original CPU register offsets");
_Static_assert(offsetof(struct port_bugcheck_debugdata,general_registers)==110u*4u,
               "Original general register offsets");
_Static_assert(offsetof(struct port_bugcheck_debugdata,flag)==127u*4u,
               "Original bugcheck flag offset");

/* MDATEND.MAR: 16 empty PFN opcode patch entries and a zero terminator. */
struct port_pfn_patch_table port_mmg_pfn_patch
    __attribute__((section(".vms_pfn_fixup")));
struct port_pfn_patch_descriptor port_mmg_pfn_patch_desc={
    16u*6u, port_mmg_pfn_patch.entries
};
_Static_assert(sizeof(struct port_pfn_patch)==6u,"PFN fixup entry is six bytes");
_Static_assert(sizeof(struct port_pfn_patch_table)==100u,"PFN fixup list terminator");

/* The final SYS.EXE linker script must place this marker last. */
__asm__(".pushsection .vms_sys_end,\"a\",@progbits\n"
        ".balign 4096\n"
        ".globl port_mmg_sys_end\n"
        "port_mmg_sys_end:\n"
        ".popsection\n");

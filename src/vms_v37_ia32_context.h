#ifndef VMS_V37_IA32_CONTEXT_H
#define VMS_V37_IA32_CONTEXT_H
#include <stddef.h>
#include <stdint.h>
#include "vms_pcb.h"

enum {
    PORT_V37_IA32_CONTEXT_OK=1u,
    PORT_V37_IA32_CONTEXT_BAD=2u,
    PORT_V37_IA32_CONTEXT_PRIVILEGE=4u,
    PORT_V37_IA32_CONTEXT_FLAGS=6u,
    PORT_V37_IA32_CONTEXT_STACK=8u
};

/* Stack image consumed by arch_isr_common's segment pops, POPAL and IRET.
 * For CPL0 IRET uses EIP/CS/EFLAGS; for CPL1-3 it also consumes ESP/SS. */
struct port_v37_ia32_restore_frame {
    uint32_t gs,fs,es,ds;
    struct port_trap_frame registers;
};

struct port_v37_ia32_context_seed {
    uint32_t cr3,entry_pc,initial_stack,eflags,initial_kernel_argument;
    uint16_t code_selector,data_selector;
    uint8_t access_mode;
    uint8_t reserved;
};

/* Construct the initial native saved frame at the top of a process-owned
 * kernel stack. VMS access modes map to matching IA-32 CPL0-CPL3. For a
 * CPL0 process, the post-IRET stack contains a null return address followed
 * by initial_kernel_argument, mapping source R4 to the C entry argument.
 * Nonzero modes require caller-mapped stack and page tables. */
uint32_t port_v37_ia32_context_init(void *kernel_stack,size_t stack_bytes,
    const struct port_v37_ia32_context_seed *,
    struct port_v37_ia32_restore_frame **restore_frame);
/* Validate a saved frame before SCH$SCHED hands it to the IA-32 IRET path. */
uint32_t port_v37_ia32_context_validate(
    const struct port_v37_ia32_restore_frame *restore_frame);
/* Convert a complete saved frame extent into a 32-bit TSS ESP0, rejecting
 * address-space wrap before any privilege-stack register is changed. */
uint32_t port_v37_ia32_context_stack_top(uintptr_t frame_address,
    size_t frame_bytes,uint32_t *stack_top);

#endif

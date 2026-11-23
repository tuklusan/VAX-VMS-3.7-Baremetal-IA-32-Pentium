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
 * For CPL0 IRET uses EIP/CS/EFLAGS; for CPL3 it also consumes user ESP/SS. */
struct port_v37_ia32_restore_frame {
    uint32_t gs,fs,es,ds;
    struct port_trap_frame registers;
};

struct port_v37_ia32_context_seed {
    uint32_t cr3,entry_pc,initial_stack,eflags;
    uint16_t code_selector,data_selector;
    uint8_t access_mode;
    uint8_t reserved;
};

/* Construct the initial native saved frame at the top of a process-owned
 * kernel stack. The returned pointer names registers, as arch_trap expects. */
uint32_t port_v37_ia32_context_init(void *kernel_stack,size_t stack_bytes,
    const struct port_v37_ia32_context_seed *,
    struct port_v37_ia32_restore_frame **restore_frame);

#endif

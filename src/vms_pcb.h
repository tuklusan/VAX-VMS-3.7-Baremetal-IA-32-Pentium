#ifndef PORTLAB_VMS_PCB_H
#define PORTLAB_VMS_PCB_H
#include <stdint.h>
#include "vms_ast.h"

/* Hardware frame captured by arch_isr_common: pushal, vector/error, IRET.
 * user_esp/user_ss are present only for CPL3 -> CPL0 transitions. */
struct port_trap_frame {
    uint32_t edi, esi, ebp, esp_at_pushal, ebx, edx, ecx, eax;
    uint32_t vector, error, eip, cs, eflags, user_esp, user_ss;
};
_Static_assert(sizeof(struct port_trap_frame)==60, "IA-32 trap frame ABI");

enum port_pcb_state { PORT_PCB_FREE, PORT_PCB_READY,
                      PORT_PCB_RUNNING, PORT_PCB_WAIT_EF, PORT_PCB_EXITED,
                      PORT_PCB_WAIT_RES };
struct port_pcb {
    uint32_t pid;
    enum port_pcb_state state;
    uint8_t started;
    uint32_t *cr3;             /* supervised page directory */
    uint32_t user_stack_top;
    uint32_t user_entry;
    uintptr_t kernel_stack_base;
    uintptr_t kernel_stack_top;
    uint32_t kernel_entries;
    uint32_t private_frame;    /* owned 4-KiB frame, or zero */
    uint32_t wait_mask;
    uint8_t wait_all;
    uint32_t cpu_ticks, dispatches, reports, memory_report, memory_report2;
    struct port_trap_frame frame;
    struct port_trap_frame ast_resume;
    struct vms_ast_process ast;
    struct vms_ast_request *active_ast;
};
int port_pcb_create(struct port_pcb *p, uint32_t pid, uint32_t *cr3,
                    uint32_t user_stack_top, uint32_t user_entry);
int port_pcb_bind_kernel_stack(struct port_pcb *p, uintptr_t base, uintptr_t top);
int port_pcb_activate(struct port_pcb *p);
int port_pcb_deschedule(struct port_pcb *p);
int port_pcb_exit(struct port_pcb *p);
void port_pcb_tick(struct port_pcb *p);
int port_pcb_block_ef(struct port_pcb *p,uint32_t mask,int all);
int port_pcb_wake_ef(struct port_pcb *p);
int port_pcb_block_res(struct port_pcb *p);
int port_pcb_wake_res(struct port_pcb *p);
int port_pcb_consistent(const struct port_pcb *p);
#endif

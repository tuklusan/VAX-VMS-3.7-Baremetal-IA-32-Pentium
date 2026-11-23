#include "vms_pcb.h"

int port_pcb_create(struct port_pcb *p, uint32_t pid, uint32_t *cr3,
                    uint32_t user_stack_top, uint32_t user_entry) {
    if (!p || !pid || !cr3 || ((uintptr_t)cr3&4095u) ||
        (user_stack_top&3u) || !user_entry) return -1;
    p->pid=pid;
    p->state=PORT_PCB_READY;
    p->started=0;
    p->cr3=cr3;
    p->user_stack_top=user_stack_top;
    p->user_entry=user_entry;
    p->kernel_stack_base=p->kernel_stack_top=0;
    p->kernel_entries=0;
    p->private_frame=0;
    p->wait_mask=0; p->wait_all=0;
    p->cpu_ticks=p->dispatches=p->reports=p->memory_report=p->memory_report2=0;
    p->active_ast=0;
    vms_ast_init(&p->ast,pid);
    return 0;
}
int port_pcb_bind_kernel_stack(struct port_pcb *p, uintptr_t base, uintptr_t top) {
    if (!p || !p->pid || p->state!=PORT_PCB_READY ||
        (base&4095u) || top-base!=4096u || top<=base) return -1;
    p->kernel_stack_base=base;
    p->kernel_stack_top=top;
    p->kernel_entries=0;
    return 0;
}
int port_pcb_activate(struct port_pcb *p) {
    if (!p || (p->state!=PORT_PCB_READY && p->state!=PORT_PCB_RUNNING))
        return -1;
    p->state=PORT_PCB_RUNNING;
    ++p->dispatches;
    return 0;
}
int port_pcb_deschedule(struct port_pcb *p) {
    if (!p || p->state!=PORT_PCB_RUNNING) return -1;
    p->state=PORT_PCB_READY;
    return 0;
}
int port_pcb_exit(struct port_pcb *p) {
    if (!p || p->state!=PORT_PCB_RUNNING) return -1;
    p->state=PORT_PCB_EXITED;
    return 0;
}
int port_pcb_block_ef(struct port_pcb *p,uint32_t mask,int all) {
    if (!p || p->state!=PORT_PCB_RUNNING || !mask) return -1;
    p->wait_mask=mask;
    p->wait_all=all!=0;
    p->state=PORT_PCB_WAIT_EF;
    return 0;
}
int port_pcb_wake_ef(struct port_pcb *p) {
    if (!p || p->state!=PORT_PCB_WAIT_EF) return -1;
    p->wait_mask=0; p->wait_all=0;
    p->state=PORT_PCB_READY;
    return 0;
}
int port_pcb_block_res(struct port_pcb *p) {
    if(!p || p->state!=PORT_PCB_RUNNING)return -1;
    p->state=PORT_PCB_WAIT_RES;return 0;
}
int port_pcb_wake_res(struct port_pcb *p) {
    if(!p || p->state!=PORT_PCB_WAIT_RES)return -1;
    p->state=PORT_PCB_READY;return 0;
}
void port_pcb_tick(struct port_pcb *p) {
    if (p && p->state==PORT_PCB_RUNNING) ++p->cpu_ticks;
}
int port_pcb_consistent(const struct port_pcb *p) {
    if (!p || !p->pid || !p->cr3 || ((uintptr_t)p->cr3&4095u) ||
        !p->user_entry || (p->user_stack_top&3u) ||
        p->state==PORT_PCB_FREE || p->state>PORT_PCB_WAIT_RES ||
        p->ast.pid!=p->pid ||
        (p->state==PORT_PCB_WAIT_EF && !p->wait_mask) ||
        (p->kernel_stack_top &&
         ((p->kernel_stack_base&4095u) ||
          p->kernel_stack_top-p->kernel_stack_base!=4096u))) return 0;
    return p->started || p->state==PORT_PCB_READY;
}

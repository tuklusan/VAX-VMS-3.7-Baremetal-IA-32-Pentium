#include "vms_sysloa386.h"
/* Hardware replacement for SYSLOAVEC's EXE$INIPROCREG; the VAX IPL,
 * SCBB, MAPEN and machine-check registers are *not* x86 CPU registers.
 * The live x86 IDTR, CR0, CR3 and TSS must be initialized by arch_init. */
static uint32_t gate_target(const struct port_sysloa386_gate *g){
    return (uint32_t)g->offset_low|((uint32_t)g->offset_high<<16);
}
static int gate_ok(const struct port_sysloa386_gate *g){
    const uint32_t at=gate_target(g);
    return g->selector==8u && g->reserved==0u &&
           (g->type==0x8eu) && at>=0x10000u && at<0x00320000u;
}
uint32_t port_sysloa386_check_cpu(const struct port_sysloa386_cpu_result *r){
    if(!r || (r->cr0&0x80010001u)!=0x80010001u ||
       !(r->cr3&0xfffff000u) || (r->cr3&4095u)!=0u ||
       r->idt_base<0x10000u || r->idt_base>=0x00320000u ||
       r->idt_limit<32u*8u+7u || r->task_selector!=0x48u ||
       r->kernel_cs!=8u || !r->irq_ticks ||
       !gate_ok(&r->gp) || !gate_ok(&r->pf) || !gate_ok(&r->timer))
        return PORT_SYSLOA386_BAD_CPU;
    return PORT_SYSLOA386_OK;
}
uint32_t port_sysloa386_cpu_probe(const struct port_sysloa386_cpu_ops *ops,
                                   struct port_sysloa386_cpu_result *r){
    if(!ops||!ops->read_irq_ticks||!r)return PORT_SYSLOA386_BAD_ARGUMENT;
#if defined(__i386__) && defined(PORT_SYSLOA386_GUEST)
    struct {uint16_t limit;uint32_t base;} __attribute__((packed)) idtr;
    __asm__ volatile("sidt %0":"=m"(idtr));
    r->idt_base=idtr.base;r->idt_limit=idtr.limit;
    __asm__ volatile("movl %%cr0,%0":"=r"(r->cr0));
    __asm__ volatile("movl %%cr3,%0":"=r"(r->cr3));
    __asm__ volatile("str %0":"=r"(r->task_selector));
    __asm__ volatile("movw %%cs,%0":"=r"(r->kernel_cs));
    r->reserved=0u;
    if(r->idt_base<0x10000u || r->idt_base>=0x00320000u ||
       r->idt_limit<32u*8u+7u)return PORT_SYSLOA386_BAD_CPU;
    const struct port_sysloa386_gate *idt=
        (const struct port_sysloa386_gate *)(uintptr_t)r->idt_base;
    r->gp=idt[13];r->pf=idt[14];r->timer=idt[32];
    r->irq_ticks=ops->read_irq_ticks(ops->context);
    return port_sysloa386_check_cpu(r);
#else
    /* User-mode host code cannot lawfully read CR0 or CR3. Only the pure
     * checker is host-tested; actual register inspection runs in QEMU. */
    (void)r;
    return PORT_SYSLOA386_BAD_CPU;
#endif
}

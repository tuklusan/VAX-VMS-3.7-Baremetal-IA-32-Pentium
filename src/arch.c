/* Native IA-32 IRQ, TSS, ring-3 trap handling and page permissions.
 * Freestanding experimental code, not the original VMS executive. */
#include <stdint.h>
#include <stddef.h>
#include "vms_condition.h"
#include "vms_ast.h"
#include "vms_pmap.h"
#include "vms_pcb.h"
#include "vms_phys.h"
#include "vms_ramcheck.h"
#include "vms_kstack.h"
#include "vms_vmem.h"
#include "vms_event.h"
#include "vms_postef.h"
#include "vms_mutex.h"
#include "vms_userptr.h"
#include "vms_userio.h"
#include "vms_channels.h"
#include "vms_exe_gettim.h"
#include "vms_exe_sched.h"
#include "vms_logintime.h"
#include "vms_iopost_swi.h"
#include "vms_sysboot_elf.h"
#include "vms_dq_crb.h"
#include "vms_v37_ia32_context.h"
#include "vms_v37_sched.h"

extern const uint32_t arch_isr_table[37];
extern uint32_t port_gdt[];
extern void arch_load_tr(void);
extern void arch_enter_ring3(void (*entry)(void), void *stack_top, uint32_t parameter);
extern void arch_user_basic(void);
extern void arch_user_ptrprobe(void);
extern void arch_user_channel_probe(void);
extern void arch_user_qio_probe(void);
extern void arch_user_qio_cancel_probe(void);
extern void arch_user_qio_teardown_probe(void);
extern void arch_user_copyin_probe(void);
extern void arch_user_qio_desc_probe(void);
extern int port_ata_x86_read28(uint32_t lba,uint16_t dst[256]);
extern int port_ata_x86_init(void);
extern void arch_user_fault(void);
extern void arch_user_ast(void);
extern void arch_user_code_begin(void);
extern void arch_user_code_end(void);
extern void arch_user_task_a(void);
extern void arch_user_task_b(void);
extern void arch_user_preempt_a(void);
extern void arch_user_preempt_b(void);
extern void arch_user_auto_ast(void);
extern void arch_user_isolation(void);
extern void arch_user_mem_a(void);
extern void arch_user_mem_b(void);
extern void arch_user_demand_a(void);
extern void arch_user_demand_b(void);
extern void arch_user_event_a(void);
extern void arch_user_event_b(void);
extern void arch_user_mutex_a(void);
extern void arch_user_mutex_b(void);
extern void arch_user_iso_fault_site(void);
extern void arch_user_iso_fault_after(void);
extern void arch_user_fault_site(void);
extern void arch_user_fault_after(void);
extern void arch_install_gdt(void);
extern void arch_report_fault(uint32_t vector, uint32_t error, uint32_t eip);
extern struct port_condition_process portlab_conditions;
extern void arch_probe_ud2(void);
extern void arch_probe_int3_after(void);
extern void arch_resume_initial_frame(
    struct port_v37_ia32_restore_frame *frame);
extern void arch_vms_ring1_probe_entry(void);
extern void arch_vms_ring2_probe_entry(void);
extern void arch_vms_ring_probe_exit(void);
extern struct port_v37_ia32_restore_frame *arch_selected_restore_frame;

static volatile uint32_t ticks;
enum { ARCH_TRAP_RETURN_CURRENT=0,ARCH_TRAP_RETURN_RING3=1,
       ARCH_TRAP_RESTORE_SELECTED_FRAME=2 };
static volatile uint32_t vms_ring_probe_active;
static volatile uint32_t vms_ring_probe_seen[3];
static uint8_t vms_ring_probe_stack[2][512] __attribute__((aligned(16)));
static struct port_v37_ia32_restore_frame *vms_ring_probe_frames[3];
static struct port_v37_scheduler *resident_vms_scheduler;
static port_v37_sched_interrupt_fn resident_vms_sched_interrupt;

int arch_adopt_vms_scheduler(struct port_v37_scheduler *scheduler,
                             port_v37_sched_interrupt_fn interrupt){
    if(!scheduler||!interrupt||resident_vms_scheduler||
       resident_vms_sched_interrupt)return 0;
    uint32_t flags;
    __asm__ volatile("pushfl; popl %0; cli":"=r"(flags)::"memory");
    if(resident_vms_scheduler||resident_vms_sched_interrupt){
        if(flags&0x200u)__asm__ volatile("sti":::"memory");
        return 0;
    }
    resident_vms_scheduler=scheduler;
    resident_vms_sched_interrupt=interrupt;
    __asm__ volatile("":::"memory");
    if(flags&0x200u)__asm__ volatile("sti":::"memory");
    return 1;
}

/* Prepare the common interrupt-return path for a scheduler-selected VMS
 * context. The trap dispatcher must still explicitly return
 * ARCH_TRAP_RESTORE_SELECTED_FRAME before assembly consumes this frame. */
int arch_prepare_restore_frame(
    struct port_v37_ia32_restore_frame *frame){
    if(port_v37_ia32_context_validate(frame)!=PORT_V37_IA32_CONTEXT_OK)
        return 0;
    arch_selected_restore_frame=frame;
    return 1;
}

int arch_vms_ring_probe(void){
    uint32_t cr3;
    __asm__ volatile("movl %%cr3,%0":"=r"(cr3));
    vms_ring_probe_seen[1]=vms_ring_probe_seen[2]=0u;
    vms_ring_probe_active=1u;
    const struct port_v37_ia32_context_seed seeds[2]={
        {.cr3=cr3,.entry_pc=(uint32_t)(uintptr_t)arch_vms_ring1_probe_entry,
         .initial_stack=(uint32_t)(uintptr_t)(vms_ring_probe_stack[0]+sizeof vms_ring_probe_stack[0]),
         .eflags=2u,.code_selector=0x19u,.data_selector=0x21u,.access_mode=1u},
        {.cr3=cr3,.entry_pc=(uint32_t)(uintptr_t)arch_vms_ring2_probe_entry,
         .initial_stack=(uint32_t)(uintptr_t)(vms_ring_probe_stack[1]+sizeof vms_ring_probe_stack[1]),
         .eflags=2u,.code_selector=0x2au,.data_selector=0x32u,.access_mode=2u}
    };
    int valid=1;
    for(unsigned i=0u;i<2u;i++){
        struct port_v37_ia32_restore_frame *frame=0;
        if(port_v37_ia32_context_init(vms_ring_probe_stack[i],
            sizeof vms_ring_probe_stack[i],&seeds[i],&frame)!=
            PORT_V37_IA32_CONTEXT_OK){valid=0;break;}
        vms_ring_probe_frames[i+1u]=frame;
    }
    if(valid){
        if(!arch_prepare_restore_frame(vms_ring_probe_frames[2]))
            valid=0;
    }
    if(valid){
        arch_resume_initial_frame(vms_ring_probe_frames[1]);
        if(vms_ring_probe_seen[1]!=1u||vms_ring_probe_seen[2]!=1u)
            valid=0;
    }
    vms_ring_probe_active=0u;
    return valid;
}
/* A real 8254 PIT timebase read by loaded SYSLOA386. */
uint32_t arch_pit_ticks(void){return ticks;}
static struct port_exe_systime exe_system_clock;
static volatile uint32_t exe_clock_valid;
/* After INIT hands off, the actual SYS.EXE residence owns time.  The
 * kernel's early clock is used only until that handoff completes. */
static const struct port_exe_systime *volatile resident_exe_clock;
static void (*volatile resident_exe_tick)(void);
/* Keep pointer+callback assignment atomic with respect to IRQ0.  A caller
 * must validate both addresses against the loaded SYS.EXE mapped region. */
int arch_adopt_executive_clock(const struct port_exe_systime *clock,
                               void (*tick)(void)) {
    if(!clock || !tick || !exe_clock_valid || resident_exe_clock ||
       (clock->sequence&1u))return 0;
    uint32_t flags;
    __asm__ volatile("pushfl; popl %0; cli":"=r"(flags)::"memory");
    if(resident_exe_clock){
        if(flags&0x200u)__asm__ volatile("sti":::"memory");
        return 0;
    }
    resident_exe_tick=tick;
    resident_exe_clock=clock;
    if(flags&0x200u)__asm__ volatile("sti":::"memory");
    return 1;
}
/* A failed RTC read cannot create a made-up absolute system date. */
const struct port_exe_systime *arch_exe_systime(void) {
    if(resident_exe_clock)return resident_exe_clock;
    return exe_clock_valid ? &exe_system_clock : 0;
}

/* Source SCBVECTOR.MAR + IOCIOPOST.MAR: a real IA-32 CPL0 software
 * interrupt, not a pretend VAX PSL or PIC hardware IRQ. */
static struct port_iopost_swi resident_iopost_irq;
static void (*disk_irq_handler)(void);
static struct port_dq_crb *volatile disk_irq_crb;
static volatile uint32_t disk_irq_bind_faults;
int arch_disk_irq_bind(void (*handler)(void)) {
    if(!handler||(disk_irq_handler&&disk_irq_handler!=handler))return 0;
    disk_irq_handler=handler;
    return 1;
}
int arch_disk_irq_bind_crb(struct port_dq_crb *crb){
    if(port_dq_crb_validate(crb)!=PORT_DQ_CRB_OK||
       crb->irq_handler!=disk_irq_handler||
       (disk_irq_crb&&disk_irq_crb!=crb))return 0;
    uint32_t flags;
    __asm__ volatile("pushfl; popl %0; cli":"=r"(flags)::"memory");
    disk_irq_crb=crb;
    if(flags&0x200u)__asm__ volatile("sti":::"memor
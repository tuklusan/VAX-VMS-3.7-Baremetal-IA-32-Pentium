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

extern const uint32_t arch_isr_table[38];
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
static int arch_prepare_vms_restore_frame(
    struct port_v37_ia32_restore_frame *frame);
static void monitor_kernel_stack(void);
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
static uint32_t (*resident_vms_sched_wait)(struct port_v37_scheduler *,
    uint32_t,uint32_t,uint32_t *,uint32_t *);
static uint32_t resident_vms_null_pcb;
static void (*resident_vms_schedule_message)(const char *);
static volatile uint32_t resident_vms_reschedule_pending;
static volatile uint32_t resident_vms_dispatch_deferred;
static volatile uint32_t resident_vms_reschedule_requests;
static volatile uint32_t resident_vms_reschedule_interrupts;
static volatile uint32_t resident_vms_wait_queue;
static uint32_t vms_process_directories[3];
static uint32_t vms_process_directory_count;

int arch_adopt_vms_scheduler(struct port_v37_scheduler *scheduler,
                             port_v37_sched_interrupt_fn interrupt,
                             uint32_t (*wait_current)(struct port_v37_scheduler *,
                                 uint32_t,uint32_t,uint32_t *,uint32_t *),
                             uint32_t null_pcb,
                             void (*schedule_message)(const char *)){
    if(!scheduler||!interrupt||!wait_current||!null_pcb||
       !schedule_message||resident_vms_scheduler||
       resident_vms_sched_interrupt)return 0;
    uint32_t flags;
    __asm__ volatile("pushfl; popl %0; cli":"=r"(flags)::"memory");
    if(resident_vms_scheduler||resident_vms_sched_interrupt){
        if(flags&0x200u)__asm__ volatile("sti":::"memory");
        return 0;
    }
    resident_vms_scheduler=scheduler;
    resident_vms_sched_interrupt=interrupt;
    resident_vms_sched_wait=wait_current;
    resident_vms_null_pcb=null_pcb;
    resident_vms_schedule_message=schedule_message;
    resident_vms_reschedule_pending=0u;
    __asm__ volatile("":::"memory");
    if(flags&0x200u)__asm__ volatile("sti":::"memory");
    return 1;
}

/* SCH$RESCHED's UP native hook. A software interrupt is raised only after
 * an IPL_SYNCH transaction has restored the caller's interrupt state. */
void arch_request_vms_reschedule(void){
    uint32_t flags;
    __asm__ volatile("pushfl; popl %0":"=r"(flags)::"memory");
    if(!resident_vms_scheduler||!resident_vms_sched_interrupt)return;
    if(resident_vms_reschedule_requests!=UINT32_MAX)
        resident_vms_reschedule_requests++;
    resident_vms_reschedule_pending=1u;
    if((flags&UINT32_C(0x200))&&!resident_vms_dispatch_deferred){
        resident_vms_reschedule_pending=0u;
        __asm__ volatile("int $0x91":::"memory");
    }
}

/* INIT may finish binding several queued source PCBs while kmain still owns
 * the only resumable bootstrap frame. Defer dispatch until that transaction
 * is validated and kmain explicitly yields. IRQ work and clock ticks remain
 * enabled; only the scheduler selection is held. */
void arch_vms_scheduler_defer_dispatch(uint32_t defer){
    uint32_t flags;
    __asm__ volatile("pushfl; popl %0; cli":"=r"(flags)::"memory");
    resident_vms_dispatch_deferred=defer?1u:0u;
    __asm__ volatile("":::"memory");
    if(flags&UINT32_C(0x200))__asm__ volatile("sti":::"memory");
}

uint32_t arch_vms_reschedule_requests(void){
    return resident_vms_reschedule_requests;
}
uint32_t arch_vms_reschedule_interrupts(void){
    return resident_vms_reschedule_interrupts;
}

void arch_run_pending_vms_reschedule(void){
    uint32_t flags;
    __asm__ volatile("pushfl; popl %0":"=r"(flags)::"memory");
    if(!(flags&UINT32_C(0x200))||resident_vms_dispatch_deferred||
       !resident_vms_reschedule_pending||
       !resident_vms_scheduler||!resident_vms_sched_interrupt)return;
    resident_vms_reschedule_pending=0u;
    __asm__ volatile("int $0x91":::"memory");
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
        /* The probe selected a private VMS-mode stack for its nested gate;
         * restore the monitor's TSS stack before ordinary boot continues. */
        monitor_kernel_stack();
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
    if(flags&0x200u)__asm__ volatile("sti":::"memory");
    return 1;
}
uint32_t arch_disk_irq_crb_count(void){
    struct port_dq_crb *crb=disk_irq_crb;
    return crb&&port_dq_crb_validate(crb)==PORT_DQ_CRB_OK?
        crb->interrupts:0u;
}
uint32_t arch_iopost_bind(uint32_t (*callback)(void)){
    uintptr_t address=(uintptr_t)callback;
    if(address>UINT32_MAX)return PORT_IOPOST_SW_BAD;
    return port_iopost_swi_bind(&resident_iopost_irq,callback,
       (uint32_t)address,PORT_SYS_ELF_START,PORT_SYS_ELF_LIMIT);
}
uint32_t arch_iopost_trigger(void){
    if(!resident_iopost_irq.armed)return PORT_IOPOST_SW_UNBOUND;
    /* Interrupt gate has DPL0; a ring3 INT 0x90 causes #GP. */
    __asm__ volatile("int $0x90":::"memory");
    return port_iopost_swi_check(&resident_iopost_irq,1u,PORT_IOPOST_SW_OK);
}
uint32_t arch_iopost_count(void){return resident_iopost_irq.calls;}
uint32_t arch_iopost_last_status(void){return resident_iopost_irq.last_status;}
static volatile uint32_t last_vector;
static volatile uint32_t user_cpl_report;
static volatile uint32_t user_syscalls;
static volatile uint32_t user_cancel_result, user_cancel_calls;
static volatile uint32_t user_teardown_result;
static volatile uint32_t user_faults;
static volatile uint32_t user_exits;
static volatile uint32_t user_ast_value;
static volatile uint32_t user_ast_mode;
static volatile uint32_t user_test_active;
static volatile uint32_t user_pointer_result, user_pointer_calls;
static volatile uint32_t user_copyin_result,user_copyin_success,user_copyin_match;
static struct port_channel_table arch_service_channels;
static volatile uint32_t user_channel_result,user_channel_calls;
static int arch_service_read_stub(void *ctx,uint32_t lba,uint16_t b[256]) {
    (void)ctx;
    return port_ata_x86_read28(lba,b);
}
/* M34: transactional DATA+IOSB completion on owned block request.
 * M29: independent kernel-owned single outstanding block request.
 * User buffers stay in the caller's address space for submit->wait;
 * no process exit or user unmap support in this prototype. */
static struct port_qio_service arch_service_qio;
static struct port_qio_request arch_io_request;
static struct port_qio_iosb arch_io_iosb;
static struct port_event_cluster arch_io_event;
static struct vms_ast_process arch_io_process;
static uint16_t arch_io_sector[256];
static uint32_t arch_io_dst,arch_io_iosb_dst;
static volatile uint32_t user_read_result,user_read_count,user_read_bytes;
static volatile uint32_t user_desc_result,user_desc_calls;

/* Identity-mapped physical RAM is kernel accessible in this bootstrap.
 * Never write an unchecked user virtual address directly at CPL0. */
static uint8_t *port_guest_page_resolver(void *unused,uint32_t physical) {
    (void)unused;
    if((physical&4095u)||physical>=0x00400000u)return 0;
    return (uint8_t *)(uintptr_t)physical;
}
static const uint32_t *port_guest_table_resolver(void *unused,uint32_t table) {
    (void)unused;
    /* Physical tables must be aligned and inside identity-mapped bootstrap RAM. */
    if((table&4095u)||table>=0x00400000u)return 0;
    return (const uint32_t *)(uintptr_t)table;
}

/* i386 32-bit TSS fixed layout. The I/O bitmap lies beyond the TSS limit,
 * so user code cannot perform direct IN/OUT. */
struct tss32 {
    uint16_t previous, rsv0;
    uint32_t esp0; uint16_t ss0, rsv1;
    uint32_t esp1; uint16_t ss1, rsv2;
    uint32_t esp2; uint16_t ss2, rsv3;
    uint32_t cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint16_t es, rsv4, cs, rsv5, ss, rsv6, ds, rsv7;
    uint16_t fs, rsv8, gs, rsv9, ldt, rsv10;
    uint16_t trap, iomap_base;
} __attribute__((packed));
_Static_assert(sizeof(struct tss32)==104, "TSS32 layout");
static struct tss32 tss;
/* When SCH$SCHED selects an access-mode 1-3 process, subsequent interrupt
 * gates enter CPL0 through this process's saved kernel-stack top. Its IRET
 * image is complete because every installed IDT gate targets selector 0x08. */
static int arch_prepare_vms_restore_frame(
    struct port_v37_ia32_restore_frame *frame){
    if(port_v37_ia32_context_validate(frame)!=PORT_V37_IA32_CONTEXT_OK)
        return 0;
    if(frame->registers.cs&3u){
        uint32_t top=0u;
        if(port_v37_ia32_context_stack_top((uintptr_t)frame,sizeof(*frame),
                                           &top)!=PORT_V37_IA32_CONTEXT_OK)
            return 0;
        tss.esp0=top;
    }
    arch_selected_restore_frame=frame;
    return 1;
}
static uint8_t privilege_stack[4096] __attribute__((aligned(16)));
/* Guard page + task-owned 4K supervisor interrupt stack for each PCB. */
static struct port_kernel_stack task_kernel_stack[2];
static volatile uint32_t kernel_stack_checks, kernel_stack_faults;
static uint8_t user_stack[4096] __attribute__((aligned(4096)));
uint8_t task_b_stack[4096] __attribute__((aligned(4096)));
static volatile uint32_t sched_switches, sched_reports, sched_done, sched_sequence;
static volatile uint32_t sched_active, sched_current;
static volatile uint32_t sched_preempt, sched_preemptions;
static volatile uint32_t sched_vm_test;
static volatile uint32_t sched_demand_test;
static volatile uint32_t sched_event_test;
static volatile uint32_t sched_mutex_test;
static struct port_mutex_service sched_mutex;
static struct port_mutex sched_test_lock;
static volatile uint32_t mutex_reports[2];
static struct port_event_cluster common_events;
static struct port_postef_service sched_postef;
static volatile uint32_t event_blocks,event_wakes,event_immediate;
static struct port_vmem_region task_demand[2];
static volatile uint32_t demand_faults, demand_zero_seen, demand_reclaimed, demand_isolated;
static volatile uint32_t vm_isolated, vm_released;
static struct port_phys_pool physical_pool;
static volatile uint32_t sched_auto_ast_reports, sched_auto_ast_completed;
static volatile uint32_t isolation_active, isolation_faults;
/* Private user-visible physical page, read-only in both task address spaces.
 * An independent supervisor-only alias at 0x400000 permits kernel updates
 * while CR0.WP remains enabled (no temporary permission relaxation). */
volatile uint32_t arch_user_clock_page[1024] __attribute__((aligned(4096)));
static uint32_t clock_alias_table[1024] __attribute__((aligned(4096)));
static uint32_t task_page_directory[2][1024] __attribute__((aligned(4096)));
static uint32_t task_page_table[2][1024] __attribute__((aligned(4096)));
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t low_page_table[1024] __attribute__((aligned(4096)));

/* Build the flat supervisor mapping used by the initial VMS system
 * processes. Their current code/data all live in the identity mapped first
 * four MiB; independent user P0/P1 image maps are still future work. */
uint32_t arch_vms_make_process_cr3(uint32_t *p0_table,uint32_t *p1_table,
                                   uint32_t *cr3_out){
    if(!p0_table||!p1_table||!cr3_out||
       ((uintptr_t)p0_table&4095u)||((uintptr_t)p1_table&4095u)||
       (uintptr_t)p0_table>UINT32_MAX-4095u||
       (uintptr_t)p1_table>UINT32_MAX-4095u||
       (uintptr_t)p0_table>=UINT32_C(0x00400000)||
       (uintptr_t)p1_table>=UINT32_C(0x00400000)||
       vms_process_directory_count>=3u)return 0u;
    const uint32_t slot=vms_process_directory_count;
    const uint32_t directory_physical=port_phys_alloc(&physical_pool);
    if(!directory_physical)return 0u;
    uint32_t *const directory=(uint32_t *)(uintptr_t)directory_physical;
    for(unsigned i=0u;i<1024u;i++){
        p0_table[i]=low_page_table[i];
        p1_table[i]=low_page_table[i];
        directory[i]=page_directory[i];
    }
    directory[0]=(uint32_t)(uintptr_t)p0_table|3u;
    vms_process_directories[slot]=directory_physical;
    vms_process_directory_count++;
    *cr3_out=directory_physical;
    return *cr3_out?1u:0u;
}

static void map_user_pages(uintptr_t start, uintptr_t end, int writable) {
    port_pmap_user_range(low_page_table,start,end,writable);
}
static void switch_directory(const uint32_t pd[1024]) {
    uintptr_t address=(uintptr_t)pd;
    __asm__ volatile ("movl %0,%%cr3" : : "r"(address) : "memory");
}
static void guard_kernel_stacks(uint32_t table[1024]) {
    for (unsigned i=0;i<2;i++)
        port_pmap_guard_page(table,port_kstack_guard(&task_kernel_stack[i]));
}
static void select_kernel_stack(unsigned task) {
    tss.esp0=(uint32_t)port_kstack_top(&task_kernel_stack[task]);
}
static void monitor_kernel_stack(void) {
    tss.esp0=(uint32_t)(uintptr_t)(privilege_stack+sizeof(privilege_stack));
}
static void task_pages_init(void) {
    for (unsigned i=0;i<2;i++) {
        port_pmap_task(task_page_table[i],
                       (uintptr_t)arch_user_code_begin,
                       (uintptr_t)arch_user_code_end,
                       i ? (uintptr_t)task_b_stack : (uintptr_t)user_stack,
                       i ? (uintptr_t)(task_b_stack+sizeof(task_b_stack))
                         : (uintptr_t)(user_stack+sizeof(user_stack)),
                       (uintptr_t)arch_user_clock_page,
                       (uintptr_t)(arch_user_clock_page+1024));
        for (unsigned j=0;j<1024;j++) task_page_directory[i][j]=0;
        task_page_directory[i][0]=(uint32_t)(uintptr_t)task_page_table[i]|7u;
        task_page_directory[i][1]=(uint32_t)(uintptr_t)clock_alias_table|3u;
        guard_kernel_stacks(task_page_table[i]);
    }
}
static void privilege_init(void) {
    uintptr_t base=(uintptr_t)&tss;
    monitor_kernel_stack();
    tss.ss0=0x10;
    tss.iomap_base=sizeof(tss);
    /* Installed into GDT selector 0x48, available to LTR. */
    port_gdt[18] = (uint32_t)((sizeof(tss)-1)&0xffffu)
                    | (uint32_t)((base&0xffffu)<<16);
    port_gdt[19] = (uint32_t)((base>>16)&0xffu) | (0x89u<<8)
                    | (uint32_t)(base&0xff000000u);
    arch_load_tr();
    for (unsigned i=0; i<1024; ++i)
        low_page_table[i]=(i<<12)|3u; /* supervisor RW, identity */
    for (unsigned i=0; i<1024; ++i) page_directory[i]=0;
    /* User text executable and read-only; one private user stack page. */
    map_user_pages((uintptr_t)arch_user_code_begin,
                   (uintptr_t)arch_user_code_end,0);
    map_user_pages((uintptr_t)user_stack,
                   (uintptr_t)(user_stack+sizeof(user_stack)),1);
    map_user_pages((uintptr_t)task_b_stack,
                   (uintptr_t)(task_b_stack+sizeof(task_b_stack)),1);
    page_directory[0]=(uint32_t)(uintptr_t)low_page_table|7u;
    /* Task PDE0 and user PTEs both need the U/S bit; the identity
     * kernel entries in the same table stay supervisor-only. */
    clock_alias_table[0]=((uint32_t)(uintptr_t)arch_user_clock_page)|3u;
    for (unsigned i=1;i<1024;i++) clock_alias_table[i]=0;
    page_directory[1]=(uint32_t)(uintptr_t)clock_alias_table|3u;
    for (unsigned i=0;i<2;i++) port_kstack_init(&task_kernel_stack[i]);
    guard_kernel_stacks(low_page_table);
    task_pages_init();
    uintptr_t pd=(uintptr_t)page_directory;
    __asm__ volatile ("movl %0,%%cr3" : : "r"(pd) : "memory");
    uintptr_t cr0;
    __asm__ volatile ("movl %%cr0,%0" : "=r"(cr0));
    cr0 |= (1u<<31) | (1u<<16);  /* PG and WP */
    __asm__ volatile ("movl %0,%%cr0" : : "r"(cr0) : "memory");
}
void arch_run_user_test(int fault) {
    user_test_active=1;
    user_cpl_report=0;
    arch_enter_ring3(fault ? arch_user_fault : arch_user_basic,
                     user_stack+sizeof(user_stack)-16,0);
    user_test_active=0;
}
void arch_run_user_ast(uint32_t parameter) {
    user_test_active=1;
    user_ast_value=0;
    user_ast_mode=0;
    arch_enter_ring3(arch_user_ast,user_stack+sizeof(user_stack)-16,parameter);
    user_test_active=0;
}
void arch_run_userptr_test(void) {
    user_test_active=1;
    user_pointer_result=0;
    user_pointer_calls=0;
    arch_enter_ring3(arch_user_ptrprobe,user_stack+sizeof(user_stack)-16,0);
    user_test_active=0;
}
void arch_run_user_copyin_test(void) {
    user_copyin_result=user_copyin_success=user_copyin_match=0;
    user_test_active=1;
    arch_enter_ring3(arch_user_copyin_probe,user_stack+sizeof(user_stack)-16,0);
    user_test_active=0;
}
uint32_t arch_user_copyin_flags(void){return user_copyin_result;}
uint32_t arch_user_copyin_successes(void){return user_copyin_success;}
uint32_t arch_user_copyin_matched(void){return user_copyin_match;}
void arch_run_user_channel_test(void) {
    port_channels_init(&arch_service_channels);
    user_test_active=1;
    user_channel_result=user_channel_calls=0;
    arch_enter_ring3(arch_user_channel_probe,user_stack+sizeof(user_stack)-16,0);
    user_test_active=0;
}
void arch_run_user_qio_test(void) {
    port_channels_init(&arch_service_channels);
    port_qio_init(&arch_service_qio,arch_service_read_stub,0);
    port_event_init(&arch_io_event);
    vms_ast_init(&arch_io_process,0x370028u);
    arch_io_request=(struct port_qio_request){0};
    arch_io_iosb=(struct port_qio_iosb){0};
    user_read_result=user_read_count=user_read_bytes=0;
    arch_io_dst=arch_io_iosb_dst=0;
    user_test_active=1;
    arch_enter_ring3(arch_user_qio_probe,user_stack+sizeof(user_stack)-16,0);
    user_test_active=0;
    (void)port_channels_teardown_owner(&arch_service_channels,&arch_service_qio,
                                       &arch_io_event,&arch_io_process);
}
void arch_run_user_qio_desc_test(void) {
    port_channels_init(&arch_service_channels);
    port_qio_init(&arch_service_qio,arch_service_read_stub,0);
    port_event_init(&arch_io_event);
    vms_ast_init(&arch_io_process,0x370028u);
    arch_io_request=(struct port_qio_request){0};
    arch_io_iosb=(struct port_qio_iosb){0};
    arch_io_dst=arch_io_iosb_dst=0;
    user_desc_result=user_desc_calls=0;
    user_test_active=1;
    arch_enter_ring3(arch_user_qio_desc_probe,user_stack+sizeof(user_stack)-16,0);
    user_test_active=0;
    (void)port_channels_teardown_owner(&arch_service_channels,&arch_service_qio,
                                       &arch_io_event,&arch_io_process);
}
uint32_t arch_user_desc_result(void){return user_desc_result;}
uint32_t arch_user_desc_calls(void){return user_desc_calls;}
void arch_run_user_qio_cancel_test(void) {
    port_channels_init(&arch_service_channels);
    port_qio_init(&arch_service_qio,arch_service_read_stub,0);
    port_event_init(&arch_io_event);
    vms_ast_init(&arch_io_process,0x370028u);
    arch_io_request=(struct port_qio_request){0};
    arch_io_iosb=(struct port_qio_iosb){0};
    arch_io_dst=arch_io_iosb_dst=0;
    user_cancel_result=user_cancel_calls=0;
    user_test_active=1;
    arch_enter_ring3(arch_user_qio_cancel_probe,user_stack+sizeof(user_stack)-16,0);
    user_test_active=0;
    (void)port_channels_teardown_owner(&arch_service_channels,&arch_service_qio,
                                       &arch_io_event,&arch_io_process);
}
void arch_run_user_qio_teardown_test(void) {
    port_channels_init(&arch_service_channels);
    port_qio_init(&arch_service_qio,arch_service_read_stub,0);
    port_event_init(&arch_io_event);
    vms_ast_init(&arch_io_process,0x370028u);
    arch_io_request=(struct port_qio_request){0};
    arch_io_iosb=(struct port_qio_iosb){0};
    arch_io_dst=arch_io_iosb_dst=0;
    user_teardown_result=0;
    user_test_active=1;
    arch_enter_ring3(arch_user_qio_teardown_probe,user_stack+sizeof(user_stack)-16,0);
    user_test_active=0;
    if(arch_io_request.state==1u && arch_service_qio.count==1u)
        user_teardown_result|=1u;
    if(!port_channels_teardown_owner(&arch_service_channels,&arch_service_qio,
                                     &arch_io_event,&arch_io_process))
        user_teardown_result|=2u;
    if(arch_io_request.state==0u && arch_service_qio.count==0u)
        user_teardown_result|=4u;
    if(arch_service_channels.assigned==arch_service_channels.deassigned &&
       arch_service_channels.assigned==1u)
        user_teardown_result|=8u;
}
uint32_t arch_user_teardown_result(void){return user_teardown_result;}
uint32_t arch_user_cancel_result(void) {return user_cancel_result;}
uint32_t arch_user_cancel_calls(void) {return user_cancel_calls;}
uint32_t arch_user_read_result(void) {return user_read_result;}
uint32_t arch_user_read_count(void) {return user_read_count;}
uint32_t arch_user_read_bytes(void) {return user_read_bytes;}
uint32_t arch_user_channel_result(void) {return user_channel_result;}
uint32_t arch_user_channel_calls(void) {return user_channel_calls;}
uint32_t arch_user_channel_assigned(void) {return arch_service_channels.assigned;}
uint32_t arch_user_channel_deassigned(void) {return arch_service_channels.deassigned;}
uint32_t arch_user_ptr_result(void) {return user_pointer_result;}
uint32_t arch_user_ptr_calls(void) {return user_pointer_calls;}
uint32_t arch_user_ast_value(void) { return user_ast_value; }
uint32_t arch_user_ast_mode(void) { return user_ast_mode; }
uint32_t arch_user_cpl(void) { return user_cpl_report; }
uint32_t arch_user_syscalls(void) { return user_syscalls; }
uint32_t arch_user_faults(void) { return user_faults; }
uint32_t arch_user_exits(void) { return user_exits; }


struct idt_gate {
    uint16_t offset_lo;
    uint16_t selector;
    uint8_t zero;
    uint8_t type;
    uint16_t offset_hi;
} __attribute__((packed));

struct descriptor_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_gate idt[256];
/* The PCB owns register/IRET context, CR3, state, ASTs, and accounting.
 * The memory manager owns physical page frames, never the PCB itself. */
static struct port_pcb task_pcb[2];
static struct port_exe_sched_process arch_sched_process[2];
static struct port_exe_scheduler arch_scheduler;
static struct port_trap_frame *arch_sched_trap_frame;
static volatile uint32_t arch_sched_bad_queue;
static struct vms_ast_request sched_ast_req;
static void ast_test_token(void *context, uint32_t parameter) {
    (void)context; (void)parameter;
    /* The callback is dispatched by arch_user_auto_ast at CPL3. */
}
static uint32_t isolation_original_word;
static void copy_frame(struct port_trap_frame *dst,const struct port_trap_frame *src) {
    const uint32_t *p=(const uint32_t *)src;
    uint32_t *q=(uint32_t *)dst;
    for (unsigned i=0;i<sizeof(*dst)/sizeof(uint32_t);i++) q[i]=p[i];
}
static unsigned sched_process_index(const struct port_exe_sched_process *p) {
    for(unsigned i=0;i<2u;i++)if(p==&arch_sched_process[i])return i;
    return 2u;
}
static void arch_sched_save_context(void *context,
                                    struct port_exe_sched_process *p) {
    (void)context;
    unsigned i=sched_process_index(p);
    if(i>=2u||!arch_sched_trap_frame){arch_sched_bad_queue=1u;return;}
    copy_frame(&task_pcb[i].frame,arch_sched_trap_frame);
}
static void arch_sched_load_context(void *context,
                                    struct port_exe_sched_process *p) {
    (void)context;
    unsigned i=sched_process_index(p);
    if(i>=2u||!arch_sched_trap_frame||
       (task_pcb[i].state!=PORT_PCB_READY&&task_pcb[i].state!=PORT_PCB_RUNNING)){
        arch_sched_bad_queue=1u;return;
    }
    struct port_pcb *pcb=&task_pcb[i];
    if(!pcb->started){
        copy_frame(&pcb->frame,arch_sched_trap_frame);
        pcb->frame.eip=pcb->user_entry;
        pcb->frame.user_esp=pcb->user_stack_top;
        pcb->frame.eax=pcb->frame.ebx=pcb->frame.ecx=0;
        pcb->frame.edx=pcb->frame.esi=pcb->frame.edi=0;
        pcb->started=1u;
    }
    if(pcb->state==PORT_PCB_READY&&port_pcb_activate(pcb)){
        arch_sched_bad_queue=1u;return;
    }
    copy_frame(arch_sched_trap_frame,&pcb->frame);
    sched_current=i;
    select_kernel_stack(i);
    if(sched_preempt||sched_vm_test||sched_demand_test||sched_event_test||sched_mutex_test)
        switch_directory(task_page_directory[i]);
    ++sched_switches;
}
static void arch_sched_idle(void *context) {
    (void)context;
    arch_sched_bad_queue=1u;
    for(;;)__asm__ volatile("cli; hlt");
}
static void arch_sched_fatal(void *context) {
    (void)context;
    arch_sched_bad_queue=1u;
}
static const struct port_exe_sched_ops arch_sched_ops={
    0,arch_sched_save_context,arch_sched_load_context,
    arch_sched_idle,arch_sched_fatal
};
static void arch_sched_queue_runnable(void) {
    for(unsigned i=0;i<2u;i++){
        if(task_pcb[i].state==PORT_PCB_READY&&
           arch_sched_process[i].state==PORT_EXE_STATE_OTHER){
            if(port_exe_sched_enqueue(&arch_scheduler,&arch_sched_process[i])!=PORT_SS_NORMAL)
                arch_sched_bad_queue=1u;
        }
    }
}
static void schedule_next(struct port_trap_frame *f, int is_exit) {
    unsigned cur=sched_current;
    struct port_pcb *old=&task_pcb[cur];
    struct port_exe_sched_process *current=&arch_sched_process[cur];
    arch_sched_trap_frame=f;
    if (is_exit) {
        if (port_pcb_exit(old)) return;
        ++sched_done;
    } else {
        if (old->state==PORT_PCB_RUNNING && port_pcb_deschedule(old)) return;
        if (old->state!=PORT_PCB_READY && old->state!=PORT_PCB_WAIT_EF &&
            old->state!=PORT_PCB_WAIT_RES)
            return;
    }
    arch_sched_queue_runnable();
    if(old->state==PORT_PCB_READY){
        if(current->state!=PORT_EXE_STATE_CURRENT)return;
        if(port_exe_sched_resched(&arch_scheduler,&arch_sched_ops)==PORT_SS_NORMAL&&
           !arch_sched_bad_queue)return;
    }else if(old->state==PORT_PCB_WAIT_EF||old->state==PORT_PCB_WAIT_RES||
             old->state==PORT_PCB_EXITED){
        if(current->state!=PORT_EXE_STATE_CURRENT)return;
        if(!arch_scheduler.ready_mask){
            copy_frame(&old->frame,f);
            current->state=PORT_EXE_STATE_OTHER;
            arch_scheduler.current=0;
        }else if(port_exe_sched_suspend(&arch_scheduler,&arch_sched_ops)==PORT_SS_NORMAL&&
                 !arch_sched_bad_queue)return;
    }
    if(arch_sched_bad_queue){
        arch_report_fault(f->vector,f->error,f->eip);
        for(;;)__asm__ volatile("cli; hlt");
    }
    if(old->state!=PORT_PCB_READY){
        /* The diagnostic runner has no idle PCB; return to its supervisor
         * after the final runnable test process blocks or exits. */
        sched_active=0;
        monitor_kernel_stack();
        if (sched_preempt || sched_vm_test || sched_demand_test || sched_event_test || sched_mutex_test) switch_directory(page_directory);
        return;
    }
}
/* Asynchronous user ASTs are owned by the PCB, including their suspended
 * preemption context; switching CR3 never switches another task's AST. */
static void ast_on_user_return(struct port_trap_frame *f) {
    if (!sched_active || !sched_preempt || (f->cs&3u)!=3u) return;
    struct port_pcb *pcb=&task_pcb[sched_current];
    if (pcb->active_ast) return;
    struct vms_ast_request *r=0;
    if (vms_ast_take(&pcb->ast,VMS_MODE_USER,&r)!=VMS_AST_OK) return;
    copy_frame(&pcb->ast_resume,f);
    pcb->active_ast=r;
    f->eip=(uint32_t)(uintptr_t)arch_user_auto_ast;
    f->esi=r->parameter;
}
static void sched_reset(void) {
    for(unsigned i=0;i<2;i++) {
        uint32_t entry=(uint32_t)(uintptr_t)(sched_mutex_test ?
            (i ? arch_user_mutex_b : arch_user_mutex_a) : sched_event_test ?
            (i ? arch_user_event_b : arch_user_event_a) : sched_demand_test ?
            (i ? arch_user_demand_b : arch_user_demand_a) : sched_vm_test ?
            (i ? arch_user_mem_b : arch_user_mem_a) : sched_preempt ?
            (i ? arch_user_preempt_b : arch_user_preempt_a) :
            (i ? arch_user_task_b : arch_user_task_a));
        uint32_t *pd=(sched_preempt || sched_vm_test || sched_demand_test || sched_event_test || sched_mutex_test) ?
            task_page_directory[i] : page_directory;
        (void)port_pcb_create(&task_pcb[i],0x370050u+i,pd,
            (uint32_t)(uintptr_t)(i ? task_b_stack+sizeof(task_b_stack)-16 :
                                       user_stack+sizeof(user_stack)-16),entry);
        port_kstack_init(&task_kernel_stack[i]);
        (void)port_pcb_bind_kernel_stack(&task_pcb[i],
            port_kstack_base(&task_kernel_stack[i]),
            port_kstack_top(&task_kernel_stack[i]));
    }
    port_event_init(&common_events);
    port_postef_init(&sched_postef);
    (void)port_postef_bind_common(&sched_postef,2,&common_events);
    for (unsigned i=0;i<2;i++) {
        (void)port_postef_register(&sched_postef,&task_pcb[i]);
        (void)port_postef_attach_common(&sched_postef,task_pcb[i].pid,2);
    }
    port_mutex_init(&sched_mutex);
    port_mutex_init_one(&sched_test_lock);
    for(unsigned i=0;i<2;i++)
        (void)port_mutex_register(&sched_mutex,&task_pcb[i],4+i,4+i);
    mutex_reports[0]=mutex_reports[1]=0;
    event_blocks=event_wakes=event_immediate=0;
    kernel_stack_checks=kernel_stack_faults=0;
    sched_switches=sched_reports=sched_done=sched_sequence=0;
    sched_preemptions=sched_auto_ast_reports=sched_auto_ast_completed=0;
    sched_current=0;
    task_pcb[0].started=1;
    arch_sched_bad_queue=0;
    arch_sched_trap_frame=0;
    port_exe_sched_init(&arch_scheduler,0x31u);
    for(unsigned i=0;i<2u;i++){
        arch_sched_process[i]=(struct port_exe_sched_process){
            .pid=task_pcb[i].pid,.type=0x31u,.base_priority=16u,
            .current_priority=16u,.state=PORT_EXE_STATE_OTHER};
    }
    arch_sched_process[0].state=PORT_EXE_STATE_CURRENT;
    arch_scheduler.current=&arch_sched_process[0];
    if(port_exe_sched_enqueue(&arch_scheduler,&arch_sched_process[1])!=PORT_SS_NORMAL||
       port_pcb_activate(&task_pcb[0]))arch_sched_bad_queue=1u;
    sched_active=1;
    select_kernel_stack(0);
    user_test_active=1;
}
void arch_run_preempt_test(void) {
    sched_preempt=1;
    sched_vm_test=0; sched_demand_test=0; sched_event_test=0;
    sched_reset();
    sched_ast_req.next=0;
    sched_ast_req.pid=task_pcb[0].ast.pid;
    sched_ast_req.parameter=0x0037A570u;
    sched_ast_req.callback=ast_test_token;
    sched_ast_req.context=0;
    sched_ast_req.mode=VMS_MODE_USER;
    sched_ast_req.kind=VMS_AST_NORMAL;
    sched_ast_req.state=0;
    if (vms_ast_enqueue(&task_pcb[0].ast,&sched_ast_req)!=VMS_AST_OK) {
        sched_active=sched_preempt=user_test_active=0;
        monitor_kernel_stack();
        return;
    }
    switch_directory(task_page_directory[0]);
    arch_enter_ring3(arch_user_preempt_a,user_stack+sizeof(user_stack)-16,0);
    switch_directory(page_directory);
    sched_active=sched_preempt=user_test_active=0;
    monitor_kernel_stack();
}
uint32_t arch_sched_preemptions(void) {return sched_preemptions;}
uint32_t arch_sched_auto_ast_reports(void) {return sched_auto_ast_reports;}
uint32_t arch_sched_auto_ast_completed(void) {return sched_auto_ast_completed;}
uint32_t arch_sched_ast_delivered(void) {return task_pcb[0].ast.delivered;}
uint32_t arch_isolation_faults(void) {return isolation_faults;}
void arch_run_isolation_test(void) {
    isolation_original_word=*(const uint32_t *)(const void *)task_b_stack;
    isolation_faults=0;
    user_test_active=isolation_active=1;
    switch_directory(task_page_directory[0]);
    arch_enter_ring3(arch_user_isolation,user_stack+sizeof(user_stack)-16,0);
    switch_directory(page_directory);
    isolation_active=user_test_active=0;
}
uint32_t arch_isolation_intact(void) {
    return *(const uint32_t *)(const void *)task_b_stack==isolation_original_word;
}

/* Two tasks see 0x300000 at the same virtual address but map independent
 * allocated physical frames; both retain their own marker after yielding. */
void arch_run_vm_test(void) {
    const uint32_t virt=0x00300000u;
    vm_isolated=vm_released=0;
    sched_preempt=0; sched_vm_test=1; sched_demand_test=0; sched_event_test=0;
    sched_reset();
    for (unsigned i=0;i<2;i++) {
        uint32_t pa=port_phys_alloc(&physical_pool);
        if (!pa) {
            sched_active=user_test_active=sched_vm_test=0;
            monitor_kernel_stack();
            for (unsigned j=0;j<i;j++)
                (void)port_phys_free(&physical_pool,task_pcb[j].private_frame);
            return;
        }
        task_pcb[i].private_frame=pa;
        volatile uint32_t *data=(volatile uint32_t *)(uintptr_t)pa;
        for (unsigned n=0;n<1024;n++) data[n]=0;
        task_page_table[i][virt>>12]=pa|7u; /* only task i, user RW */
    }
    switch_directory(task_page_directory[0]);
    arch_enter_ring3(arch_user_mem_a,user_stack+sizeof(user_stack)-16,0);
    switch_directory(page_directory);
    vm_isolated = sched_done==2 &&
        task_pcb[0].memory_report==0xA37A0001u &&
        task_pcb[1].memory_report==0xB37B0002u &&
        *(volatile uint32_t *)(uintptr_t)task_pcb[0].private_frame==0xA37A0001u &&
        *(volatile uint32_t *)(uintptr_t)task_pcb[1].private_frame==0xB37B0002u;
    for (unsigned i=0;i<2;i++) {
        task_page_table[i][virt>>12]=virt|3u; /* supervisor identity */
        if (port_phys_free(&physical_pool,task_pcb[i].private_frame)) vm_isolated=0;
        task_pcb[i].private_frame=0;
    }
    vm_released=port_phys_available(&physical_pool)==PORT_PHYS_FRAMES;
    sched_active=user_test_active=sched_vm_test=0;
    monitor_kernel_stack();
}
/* On a supervisor-only identity mapping, zero the just-allocated frame
 * before exposing it through a user PTE. */
static void zero_physical_frame(uint32_t physical, void *ctx) {
    (void)ctx;
    volatile uint32_t *w=(volatile uint32_t *)(uintptr_t)physical;
    for (unsigned i=0;i<PORT_PAGE_BYTES/sizeof(uint32_t);i++) w[i]=0;
}
/* M8: real CPL3 read/write faults cause per-task demand-zero page installs.
 * Guest instructions retry at the same EIP; no synthetic success return. */
void arch_run_demand_test(void) {
    const uint32_t va=0x00310000u;
    demand_faults=demand_zero_seen=demand_reclaimed=demand_isolated=0;
    sched_preempt=0; sched_vm_test=0; sched_demand_test=1; sched_event_test=0;
    sched_reset();
    int reserved=0;
    for (unsigned i=0;i<2;i++) {
        if (port_vmem_reserve(&task_demand[i],task_page_table[i],va,2)!=0)
            goto cleanup;
        ++reserved;
    }
    switch_directory(task_page_directory[0]);
    arch_enter_ring3(arch_user_demand_a,user_stack+sizeof(user_stack)-16,0);
    switch_directory(page_directory);
    /* Faults happen independently for each task and each 4K page. */
    demand_faults=task_demand[0].faults+task_demand[1].faults;
    demand_zero_seen=(task_pcb[0].reports==0 && task_pcb[1].reports==0);
    if (sched_done==2 && demand_faults==4 &&
        task_pcb[0].memory_report==0xA37A0001u &&
        task_pcb[1].memory_report==0xB37B0002u &&
        task_pcb[0].memory_report2==0xA37A1001u &&
        task_pcb[1].memory_report2==0xB37B1002u &&
        demand_zero_seen &&
        port_vmem_committed(&task_demand[0])==2 &&
        port_vmem_committed(&task_demand[1])==2 &&
        task_demand[0].frames[0]!=task_demand[1].frames[0] &&
        task_demand[0].frames[1]!=task_demand[1].frames[1]) {
        demand_isolated=1;
    }
cleanup:
    switch_directory(page_directory);
    for (int i=0;i<reserved;i++)
        if (port_vmem_release(&task_demand[i],&physical_pool,task_page_table[i]))
            demand_isolated=0;
    demand_reclaimed=(port_phys_available(&physical_pool)==PORT_PHYS_FRAMES);
    sched_active=user_test_active=sched_demand_test=0;
    monitor_kernel_stack();
}
uint32_t arch_demand_faults(void) { return demand_faults; }
uint32_t arch_demand_zero_seen(void) { return demand_zero_seen; }
uint32_t arch_demand_isolated(void) { return demand_isolated; }
uint32_t arch_demand_reclaimed(void) { return demand_reclaimed; }
/* M9: a shared event flag cluster, with each waiting PCB carrying its
 * own AND/OR mask and waking only when the requested condition holds. */
void arch_run_event_test(void) {
    sched_preempt=0; sched_vm_test=0;
    sched_demand_test=0; sched_event_test=1;
    sched_reset();
    switch_directory(task_page_directory[0]);
    arch_enter_ring3(arch_user_event_a,user_stack+sizeof(user_stack)-16,0);
    switch_directory(page_directory);
    sched_active=user_test_active=sched_event_test=0;
    monitor_kernel_stack();
}
void arch_run_mutex_test(void) {
    sched_preempt=sched_vm_test=sched_demand_test=sched_event_test=0;
    sched_mutex_test=1;
    sched_reset();
    switch_directory(task_page_directory[0]);
    arch_enter_ring3(arch_user_mutex_a,user_stack+sizeof(user_stack)-16,0);
    switch_directory(page_directory);
    sched_active=user_test_active=sched_mutex_test=0;
    monitor_kernel_stack();
}
uint32_t arch_mutex_report(unsigned i) {return i<2 ? mutex_reports[i]:0;}
uint32_t arch_mutex_waits(void) {return sched_mutex.waits;}
uint32_t arch_mutex_wakes(void) {return sched_mutex.wakeups;}
uint32_t arch_mutex_remaining(void) {return port_mutex_waiting(&sched_mutex);}
uint32_t arch_mutex_held(void) {return (uint32_t)(sched_test_lock.count+1);}
uint32_t arch_event_blocks(void) {return event_blocks;}
uint32_t arch_event_wakes(void) {return event_wakes;}
uint32_t arch_event_immediate(void) {return event_immediate;}
uint32_t arch_event_flags(void) {return common_events.flags;}
uint32_t arch_event_report(unsigned i) {
    return i<2 ? task_pcb[i].memory_report : 0;
}
uint32_t arch_vm_isolated(void) {return vm_isolated;}
uint32_t arch_vm_released(void) {return vm_released;}
uint32_t arch_phys_free(void) {return port_phys_available(&physical_pool);}
uint32_t arch_phys_high_water(void) {return physical_pool.high_water;}
uint32_t arch_pcb_pid(unsigned i) {return i<2 ? task_pcb[i].pid:0;}
uint32_t arch_pcb_ticks(unsigned i) {return i<2 ? task_pcb[i].cpu_ticks:0;}
uint32_t arch_pcb_dispatches(unsigned i) {return i<2 ? task_pcb[i].dispatches:0;}
uint32_t arch_pcb_state(unsigned i) {return i<2 ? (uint32_t)task_pcb[i].state:0;}
uint32_t arch_kstack_checks(void) {return kernel_stack_checks;}
uint32_t arch_kstack_faults(void) {return kernel_stack_faults;}
uint32_t arch_kstack_entries(unsigned i) {
    return i<2 ? task_pcb[i].kernel_entries:0;
}
uint32_t arch_kstack_tss_esp0(void) {return tss.esp0;}
int arch_kstack_protected(void) {
    if (tss.esp0!=(uint32_t)(uintptr_t)(privilege_stack+sizeof(privilege_stack))) return 0;
    for (unsigned i=0;i<2;i++) {
        if (!port_kstack_intact(&task_kernel_stack[i])) return 0;
        unsigned p=(unsigned)(port_kstack_guard(&task_kernel_stack[i])>>12);
        if (low_page_table[p] || task_page_table[0][p] || task_page_table[1][p])
            return 0;
    }
    return 1;
}
uint32_t arch_pcb_cr3(unsigned i) {return i<2 ? (uint32_t)(uintptr_t)task_pcb[i].cr3:0;}
int arch_pcb_valid(void) {
    return port_pcb_consistent(&task_pcb[0]) &&
           port_pcb_consistent(&task_pcb[1]);
}

void arch_run_scheduler_test(void) {
    sched_preempt=0; sched_vm_test=0; sched_demand_test=0; sched_event_test=0;
    sched_reset();
    arch_enter_ring3(arch_user_task_a,user_stack+sizeof(user_stack)-16,0);
    sched_active=0; user_test_active=0;
    monitor_kernel_stack();
}
uint32_t arch_sched_switches(void) {return sched_switches;}
uint32_t arch_sched_reports(void) {return sched_reports;}
uint32_t arch_sched_done(void) {return sched_done;}
uint32_t arch_sched_sequence(void) {return sched_sequence;}
uint32_t arch_sched_a(void) {return task_pcb[0].reports;}
uint32_t arch_sched_b(void) {return task_pcb[1].reports;}

static inline void outb(uint16_t p, uint8_t x) {
    __asm__ volatile ("outb %0,%1" : : "a"(x), "Nd"(p));
}
static inline void io_wait(void) {
    __asm__ volatile ("outb %%al,$0x80" : : "a"((uint8_t)0));
}

static void gate_set(unsigned v, uint32_t handler, unsigned dpl) {
    idt[v].offset_lo = (uint16_t)handler;
    idt[v].selector = 0x08;
    idt[v].zero = 0;
    idt[v].type = (uint8_t)(0x8Eu | ((dpl & 3u)<<5));
    idt[v].offset_hi = (uint16_t)(handler >> 16);
}

static int arch_vms_switch_cr3(uint32_t cr3){
    if(!cr3||(cr3&UINT32_C(0x00000fff)))return 0;
    uint32_t current;
    __asm__ volatile("movl %%cr3,%0":"=r"(current));
    if(current!=cr3)
        __asm__ volatile("movl %0,%%cr3"::"r"(cr3):"memory");
    return 1;
}

static int arch_vms_schedule_frame(struct port_trap_frame *f){
    if(!resident_vms_sched_interrupt)return 0;
    if(resident_vms_dispatch_deferred)return 0;
    resident_vms_reschedule_pending=0u;
    uint32_t pcb=0u,next=0u;
    uint32_t status;
    const uint32_t wait_queue=resident_vms_wait_queue;
    if(wait_queue){
        resident_vms_wait_queue=0u;
        status=resident_vms_sched_wait(resident_vms_scheduler,wait_queue,
            (uint32_t)(uintptr_t)((uint8_t *)f-
                offsetof(struct port_v37_ia32_restore_frame,registers)),
            &pcb,&next);
    }else{
        status=resident_vms_sched_interrupt(resident_vms_scheduler,
            (uint32_t)(uintptr_t)((uint8_t *)f-
                offsetof(struct port_v37_ia32_restore_frame,registers)),
            &pcb,&next);
    }
    if(status==PORT_V37_SCHED_NO_CONTEXT||status==PORT_V37_SCHED_EMPTY)
        return 0;
    if(status==PORT_V37_SCHED_OK&&pcb&&next&&
       (!resident_vms_scheduler->current_cr3||
        arch_vms_switch_cr3(resident_vms_scheduler->current_cr3))&&
       arch_prepare_vms_restore_frame(
           (struct port_v37_ia32_restore_frame *)(uintptr_t)next)){
        if(wait_queue&&pcb==resident_vms_null_pcb&&
           resident_vms_schedule_message)
            resident_vms_schedule_message(
                "SCH$WAITK blocked the active process; SCH$SCHED selected NULL.");
        return ARCH_TRAP_RESTORE_SELECTED_FRAME;
    }
    if(wait_queue&&resident_vms_schedule_message){
        char message[]="SCH$WAITK scheduling failed with status 0.";
        const uint32_t digit=status%10u;
        message[sizeof message-3u]=(char)('0'+digit);
        resident_vms_schedule_message(message);
    }
    arch_report_fault(f->vector,f->error,f->eip);
    for(;;)__asm__ volatile("cli; hlt");
}

/* SYSWAIT's source queue transition executes on the native software
 * reschedule gate, so the saved IRET frame is the caller's live stack. */
uint32_t arch_vms_wait_current(uint32_t wait_queue){
    if(!wait_queue||!resident_vms_scheduler||
       !resident_vms_sched_interrupt||!resident_vms_scheduler->current_pcb)
        return PORT_V37_SCHED_BADPARAM;
    uint32_t flags;
    __asm__ volatile("pushfl; popl %0; cli":"=r"(flags)::"memory");
    if(resident_vms_wait_queue){
        if(flags&UINT32_C(0x200))__asm__ volatile("sti":::"memory");
        return PORT_V37_SCHED_CORRUPT;
    }
    resident_vms_wait_queue=wait_queue;
    __asm__ volatile("":::"memory");
    /* Keep the wait-list request and SCH$RESCHED trap indivisible. Enabling
     * IRQ0 between publishing this single-CPU bridge slot and INT 0x91 lets
     * a second context overwrite it before the first wait is queued. */
    __asm__ volatile("int $0x91":::"memory");
    __asm__ volatile("pushl %0; popfl"::"r"(flags):"memory","cc");
    return PORT_V37_SCHED_OK;
}

/* Probe A20 before paging, while the 1-MiB frame allocator is unused.
 * Two distinct physical bytes must remain distinct when bit 20 is set.
 * Restore both locations; do not corrupt firmware or allocator memory. */
static uint32_t a20_ready;
static uint8_t arch_phys_read8(uint32_t address) {
    uint8_t value;
    __asm__ volatile ("movb (%1),%0" : "=q"(value) : "r"(address) : "memory");
    return value;
}
static void arch_phys_write8(uint32_t address,uint8_t value) {
    __asm__ volatile ("movb %0,(%1)" : : "q"(value),"r"(address) : "memory");
}
static int arch_verify_a20(void) {
    uint8_t old_low=arch_phys_read8(0x00000500u);
    uint8_t old_high=arch_phys_read8(0x00100500u);
    arch_phys_write8(0x00000500u,0x5au);
    arch_phys_write8(0x00100500u,0xa5u);
    int distinct=(arch_phys_read8(0x00000500u)==0x5au) &&
                 (arch_phys_read8(0x00100500u)==0xa5u);
    arch_phys_write8(0x00100500u,old_high);
    arch_phys_write8(0x00000500u,old_low);
    return distinct;
}
uint32_t arch_a20_ready(void) {return a20_ready;}
static uint32_t ram_window_ready;
static uint8_t arch_ram_read(void *ctx,uint32_t addr) {
    (void)ctx;return arch_phys_read8(addr);
}
static void arch_ram_write(void *ctx,uint32_t addr,uint8_t val) {
    (void)ctx;arch_phys_write8(addr,val);
}
uint32_t arch_ram_window_ready(void) {return ram_window_ready;}


void arch_init(void) {
    __asm__ volatile("cli");
    a20_ready=(uint32_t)arch_verify_a20();
    if (!a20_ready) return; /* kmain prints fatal error, then halts. */
    const struct port_ram_probe_bus ram_bus={arch_ram_read,arch_ram_write,0};
    ram_window_ready=(uint32_t)port_ram_probe_3mib(&ram_bus);
    if (!ram_window_ready) return;

    arch_install_gdt();
    for (unsigned i = 0; i < 34; ++i) gate_set(i, arch_isr_table[i],0);
    gate_set(128, arch_isr_table[34],3);
    gate_set(144, arch_isr_table[35],0);
    gate_set(46, arch_isr_table[36],0); /* primary ATA channel IRQ14 */
    gate_set(145, arch_isr_table[37],0); /* SCH$RESCHED software request */
    port_iopost_swi_init(&resident_iopost_irq);
    struct descriptor_ptr idtr = {(uint16_t)(sizeof(idt) - 1), (uint32_t)idt};
    __asm__ volatile("lidt %0" : : "m"(idtr));
    privilege_init();
    port_phys_init(&physical_pool);

    /* Re-map 8259A IRQ0..15 away from CPU exception vectors 0..31. */
    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait();
    outb(0xA1, 0x28); io_wait();
    outb(0x21, 0x04); io_wait();
    outb(0xA1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();
    outb(0x21,0xff);outb(0xA1,0xff);
    if(!port_ata_x86_init())return;
    uint64_t start_time;
    exe_clock_valid=0;
    resident_exe_clock=0;
    resident_exe_tick=0;
    if(port_rtc_x86_now(0,&start_time)) {
        port_exe_systime_store(&exe_system_clock,start_time);
        exe_clock_valid=1;
    }
    /* Unmask PIT, cascade, and primary ATA IRQ14; keep other IRQs masked. */
    outb(0x21,0xfa);
    outb(0xA1,0xbf);
    /* Programmable Interval Timer: 1,193,182 Hz / 11,932 ~= 100 Hz */
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(11932 & 0xFF));
    outb(0x40, (uint8_t)(11932 >> 8));
    __asm__ volatile("sti");
}

static int port_ranges_overlap(uint32_t a,uint32_t alen,
                               uint32_t b,uint32_t blen) {
    /* Called after length and end-inclusive overflow validation. */
    uint32_t a_last=a+(alen-1u),b_last=b+(blen-1u);
    return a<=b_last && b<=a_last;
}
int arch_trap(struct port_trap_frame *f) {
    last_vector = f->vector;
    if (sched_active && (f->cs&3u)==3u) {
        unsigned task=sched_current;
        ++kernel_stack_checks;
        ++task_pcb[task].kernel_entries;
        if (tss.esp0!=task_pcb[task].kernel_stack_top ||
            !port_kstack_contains_frame(&task_kernel_stack[task],
                                       (uintptr_t)f,sizeof(*f)) ||
            !port_kstack_intact(&task_kernel_stack[task])) {
            ++kernel_stack_faults;
            arch_report_fault(f->vector,f->error,f->eip);
            for (;;) __asm__ volatile ("cli; hlt");
        }
    }
    if (f->vector == PORT_IOPOST_SW_VECTOR) {
        /* DPL0 gate rejects INT 0x90 from a user task in hardware.
         * Never EOI the 8259 for a software interrupt. */
        if((f->cs&3u)!=0u ||
           port_iopost_swi_dispatch(&resident_iopost_irq,f->vector)
              !=PORT_IOPOST_SW_OK){
            arch_report_fault(f->vector,f->error,f->eip);
            for(;;)__asm__ volatile("cli; hlt");
        }
        return 0;
    }
    if(f->vector==145u){
        if((f->cs&3u)!=0u){
            arch_report_fault(f->vector,f->error,f->eip);
            for(;;)__asm__ volatile("cli; hlt");
        }
        if(resident_vms_reschedule_interrupts!=UINT32_MAX)
            resident_vms_reschedule_interrupts++;
        return arch_vms_schedule_frame(f);
    }
    if (f->vector == 128) {
        if(vms_ring_probe_active&&
           (f->eax==UINT32_C(0x37510001)||f->eax==UINT32_C(0x37510002))){
            const uint32_t mode=f->eax&0xffu;
            const uint16_t expected_cs=mode==1u?0x19u:0x2au;
            if((mode!=1u&&mode!=2u)||(f->cs&3u)!=mode||
               (uint16_t)f->cs!=expected_cs){
                arch_report_fault(f->vector,f->error,f->eip);
                for(;;)__asm__ volatile("cli; hlt");
            }
            ++vms_ring_probe_seen[mode];
            if(mode==1u){
                if(!arch_prepare_vms_restore_frame(vms_ring_probe_frames[2])){
                    arch_report_fault(f->vector,f->error,f->eip);
                    for(;;)__asm__ volatile("cli; hlt");
                }
                return ARCH_TRAP_RESTORE_SELECTED_FRAME;
            }
            const uintptr_t vms_frame=(uintptr_t)f-
                offsetof(struct port_v37_ia32_restore_frame,registers);
            if(vms_frame!=(uintptr_t)vms_ring_probe_frames[2]||
               tss.esp0!=(uint32_t)(vms_frame+
                   sizeof(struct port_v37_ia32_restore_frame))){
                arch_report_fault(f->vector,f->error,f->eip);
                for(;;)__asm__ volatile("cli; hlt");
            }
            f->eip=(uint32_t)(uintptr_t)arch_vms_ring_probe_exit;
            f->cs=0x08u;
            f->eflags&=~UINT32_C(0x00027000);
            return 0;
        }
        /* DPL3 syscall gate. Only the user probe can request an EXIT.
         * Trap frames already live on the TSS-selected kernel stack. */
        if ((f->cs&3u)!=3u || !user_test_active) {
            arch_report_fault(f->vector,f->error,f->eip);
            for (;;) __asm__ volatile ("cli; hlt");
        }
        ++user_syscalls;
        if (f->eax==16u) {
            /* Ring3 user-buffer probe: check BOTH PDE and PTE U/S, R/W,
             * page crossings, and 32-bit address overflow. Never trust
             * a pointer just because CPL0 can dereference it. */
            uint32_t *pd=sched_active ? task_page_directory[sched_current] :
                                        page_directory;
            struct port_user_map m={pd,port_guest_table_resolver,0};
            int rc=port_user_range_valid(&m,f->ebx,f->ecx,(int)(f->edx&1u));
            f->eax=(uint32_t)rc;
            ++user_pointer_calls;
            return 0;
        }
        if (f->eax==17u) {
            user_pointer_result=f->ebx;
            f->eax=0;
            return 0;
        }
        if (f->eax==27u) {
            uint32_t *pd=sched_active ? task_page_directory[sched_current] :
                                        page_directory;
            struct port_user_map m={pd,port_guest_table_resolver,0};
            uint32_t value=0;
            int rc=port_user_copy_from(&m,&value,f->ebx,sizeof(value),
                                      port_guest_page_resolver,0);
            if(!rc){
                ++user_copyin_success;
                if(value==0xA37B33CCu)++user_copyin_match;
            }
            f->eax=(uint32_t)rc;
            return 0;
        }
        if (f->eax==28u) {
            user_copyin_result=f->ebx;
            f->eax=0;
            return 0;
        }
        if (f->eax==18u) {
            /* Experimental ASSIGN: 32-bit user pointer in EBX for the
             * returned process-owned channel handle. Copy out only after
             * the entire span is checked writable in this CR3. */
            ++user_channel_calls;
            uint32_t *pd=sched_active ? task_page_directory[sched_current] :
                                        page_directory;
            struct port_user_map m={pd,port_guest_table_resolver,0};
            if(port_user_range_valid(&m,f->ebx,sizeof(uint32_t),1)) {
                f->eax=(uint32_t)PORT_CHANNEL_INVALID;
                return 0;
            }
            uint32_t handle=0;
            uint32_t pid=sched_active ? task_pcb[sched_current].pid : 0x370028u;
            int rc=port_channel_assign(&arch_service_channels,pid,
                                       arch_service_read_stub,0,&handle);
            if(!rc && port_user_copy_to(&m,f->ebx,&handle,sizeof(handle),
                                      port_guest_page_resolver,0)) {
                (void)port_channel_deassign(&arch_service_channels,handle,pid);
                rc=PORT_CHANNEL_INVALID;
            }
            f->eax=(uint32_t)rc;
            return 0;
        }
        if (f->eax==19u) {
            ++user_channel_calls;
            uint32_t pid=sched_active ? task_pcb[sched_current].pid : 0x370028u;
            f->eax=(uint32_t)port_channel_deassign(&arch_service_channels,
                                                   f->ebx,pid);
            return 0;
        }
        if (f->eax==21u) {
            user_channel_result=f->ebx;
            f->eax=0;
            return 0;
        }
        if (f->eax==22u || f->eax==29u) {
            /* Original scalar submit (22) and descriptor submit (29) use
             * the same channel and kernel-buffer path. For 29, the
             * whole descriptor is copied from translated CPL3 memory. */
            uint32_t pid=sched_active ? task_pcb[sched_current].pid : 0x370028u;
            uint32_t *pd=sched_active ? task_page_directory[sched_current] :
                                        page_directory;
            struct port_user_map m={pd,port_guest_table_resolver,0};
            uint32_t data_va=f->ecx,iosb_va=f->edx,lba=f->esi;
            if(f->eax==29u) {
                ++user_desc_calls;
                struct port_user_io_desc decoded={0};
                if(port_user_io_decode(&m,f->ecx,port_guest_page_resolver,
                                       0,&decoded)) {
                    f->eax=(uint32_t)PORT_QIO_INVALID;
                    return 0;
                }
                data_va=decoded.data_va;
                iosb_va=decoded.iosb_va;
                lba=decoded.lba;
            }
            if(port_user_range_valid(&m,data_va,512u,1) ||
               port_user_range_valid(&m,iosb_va,sizeof(arch_io_iosb),1) ||
               port_ranges_overlap(data_va,512u,iosb_va,sizeof(arch_io_iosb)) ||
               port_user_outputs_disjoint(&m,data_va,512u,iosb_va,
                                          sizeof(arch_io_iosb),port_guest_page_resolver,0) ||
               arch_io_request.state ||
               port_channel_check(&arch_service_channels,f->ebx,pid)) {
                f->eax=(uint32_t)PORT_QIO_INVALID;
                return 0;
            }
            if(arch_io_process.pid!=pid) {
                f->eax=(uint32_t)PORT_QIO_INVALID;
                return 0;
            }
            int rc=port_channel_qio_submit(&arch_service_channels,
                 &arch_service_qio,f->ebx,&arch_io_request,&arch_io_iosb,
                 arch_io_sector,lba,0,&arch_io_process,0,0,0);
            if(rc) {f->eax=(uint32_t)rc;return 0;}
            arch_io_dst=data_va;
            arch_io_iosb_dst=iosb_va;
            f->eax=0;
            return 0;
        }
        if (f->eax==30u) {
            user_desc_result=f->ebx;
            f->eax=0;
            return 0;
        }
        if (f->eax==23u) {
            uint32_t pid=sched_active ? task_pcb[sched_current].pid : 0x370028u;
            if(arch_io_process.pid!=pid || arch_io_request.pid!=pid ||
               arch_io_request.channel!=f->ebx||arch_io_request.state!=1u ||
               port_channel_check(&arch_service_channels,f->ebx,pid)){
                f->eax=(uint32_t)PORT_QIO_INVALID;
                return 0;
            }
            uint32_t *pd=sched_active ? task_page_directory[sched_current] :
                                        page_directory;
            struct port_user_map m={pd,port_guest_table_resolver,0};
            if(port_user_range_valid(&m,arch_io_dst,512u,1) ||
               port_user_range_valid(&m,arch_io_iosb_dst,sizeof(arch_io_iosb),1)) {
                /* Do not write to invalid user memory; pending operation
                 * can be cancelled by channel owner after return. */
                f->eax=(uint32_t)PORT_QIO_INVALID;
                return 0;
            }
            int rc=port_qio_run_one(&arch_service_qio,&arch_io_event,&arch_io_process);
            if(rc!=1){f->eax=(uint32_t)PORT_QIO_INVALID;return 0;}
            if(arch_io_iosb.status==PORT_QIO_IOSB_SUCCESS) {
                const struct port_user_output both[2]={
                    {arch_io_dst,arch_io_sector,512u},
                    {arch_io_iosb_dst,&arch_io_iosb,sizeof(arch_io_iosb)}
                };
                /* If either output alias disappears, publish neither. */
                if(port_user_copyv_to(&m,both,2u,port_guest_page_resolver,0)) {
                    f->eax=(uint32_t)PORT_QIO_INVALID;
                    return 0;
                }
                user_read_bytes=512u;
            } else {
                user_read_bytes=0;
                if(port_user_copy_to(&m,arch_io_iosb_dst,&arch_io_iosb,
                                     sizeof(arch_io_iosb),port_guest_page_resolver,0)) {
                    f->eax=(uint32_t)PORT_QIO_INVALID;
                    return 0;
                }
            }
            ++user_read_count;
            int released=port_channel_qio_release(&arch_service_channels,&arch_io_request);
            if(released){f->eax=(uint32_t)released;return 0;}
            arch_io_dst=arch_io_iosb_dst=0;
            f->eax=(uint32_t)(arch_io_iosb.status==PORT_QIO_IOSB_SUCCESS ? 0 : -5);
            return 0;
        }
        if (f->eax==24u) {
            user_read_result=f->ebx;
            f->eax=0;
            return 0;
        }
        if (f->eax==25u) {
            /* Release a process-owned queued or already-completed block
             * operation. No direct user pointer is touched. */
            ++user_cancel_calls;
            uint32_t pid=sched_active ? task_pcb[sched_current].pid : 0x370028u;
            if(arch_io_process.pid!=pid || arch_io_request.pid!=pid ||
               arch_io_request.channel!=f->ebx ||
               (arch_io_request.state!=1u && arch_io_request.state!=2u) ||
               port_channel_check(&arch_service_channels,f->ebx,pid)) {
                f->eax=(uint32_t)PORT_QIO_INVALID;
                return 0;
            }
            int rc=0;
            if(arch_io_request.state==1u)
                rc=port_channel_qio_cancel(&arch_service_channels,
                    &arch_service_qio,f->ebx,&arch_io_request,
                    &arch_io_event,pid);
            if(!rc)rc=port_channel_qio_release(&arch_service_channels,&arch_io_request);
            if(!rc)arch_io_dst=arch_io_iosb_dst=0;
            f->eax=(uint32_t)rc;
            return 0;
        }
        if (f->eax==26u) {
            user_cancel_result=f->ebx;
            f->eax=0;
            return 0;
        }
        if (f->eax==1u) {
            user_cpl_report=f->ebx&3u;
            f->eax=0;
            return 0;
        }
        if (f->eax==2u) {
            user_ast_mode=f->cs&3u;
            user_ast_value=f->ebx;
            if (sched_active && sched_preempt &&
                task_pcb[sched_current].active_ast &&
                task_pcb[sched_current].active_ast->parameter==f->ebx)
                ++sched_auto_ast_reports;
            f->eax=0;
            return 0;
        }
        if (sched_active && sched_preempt && f->eax==6u) {
            unsigned cur=sched_current;
            if (!task_pcb[cur].active_ast ||
                vms_ast_complete(&task_pcb[cur].ast,task_pcb[cur].active_ast)!=VMS_AST_OK) {
                arch_report_fault(f->vector,f->error,f->eip);
                for (;;) __asm__ volatile ("cli; hlt");
            }
            task_pcb[cur].active_ast=0;
            ++sched_auto_ast_completed;
            copy_frame(f,&task_pcb[cur].ast_resume);
            ast_on_user_return(f);
            return 0;
        }
        if (sched_active && sched_mutex_test && f->eax==35u) {
            unsigned next=sched_current^1u;
            uint32_t operation=f->ebx;
            if(operation>2u) {f->eax=(uint32_t)-1;return 0;}
            if(operation!=2u && task_pcb[next].state!=PORT_PCB_READY &&
               task_pcb[next].state!=PORT_PCB_RUNNING) {
                f->eax=(uint32_t)-1;return 0;
            }
            int rc=operation==0u ? port_mutex_lockr(&sched_mutex,
                task_pcb[sched_current].pid,&sched_test_lock) :
                operation==1u ? port_mutex_lockw(&sched_mutex,
                task_pcb[sched_current].pid,&sched_test_lock) :
                port_mutex_lockwnowait(&sched_mutex,
                task_pcb[sched_current].pid,&sched_test_lock);
            f->eax=(uint32_t)rc;
            if(rc==PORT_MUTEX_BLOCKED)schedule_next(f,0);
            return 0;
        }
        if (sched_active && sched_mutex_test && f->eax==36u) {
            f->eax=(uint32_t)port_mutex_unlock(&sched_mutex,
                task_pcb[sched_current].pid,&sched_test_lock);
            arch_sched_queue_runnable();
            return 0;
        }
        if (sched_active && sched_mutex_test && f->eax==37u) {
            mutex_reports[sched_current]=f->ebx;
            f->eax=0;
            return 0;
        }
        if (sched_active && sched_mutex_test && f->eax==38u) {
            unsigned next=sched_current^1u;
            if(task_pcb[next].state!=PORT_PCB_READY &&
               task_pcb[next].state!=PORT_PCB_RUNNING) {
                f->eax=(uint32_t)-1;return 0;
            }
            int rc=port_mutex_rwait(&sched_mutex,
                                  task_pcb[sched_current].pid,f->ebx);
            f->eax=rc==PORT_MUTEX_BLOCKED ? 0u : (uint32_t)-1;
            if(rc==PORT_MUTEX_BLOCKED)schedule_next(f,0);
            return 0;
        }
        if (sched_active && sched_mutex_test && f->eax==39u) {
            f->eax=(uint32_t)port_mutex_ravail(&sched_mutex,f->ebx);
            return 0;
        }
        if (sched_active && sched_event_test && f->eax==10u) {
            if (!f->ebx) {f->eax=(uint32_t)-1; return 0;}
            unsigned woken=0;
            int rc=port_postef_post_mask(&sched_postef,
                task_pcb[sched_current].pid,2,f->ebx,0,&woken);
            event_wakes+=woken;
            arch_sched_queue_runnable();
            f->eax=rc>0 ? 0u : (uint32_t)-1;
            return 0;
        }
        if (sched_active && sched_event_test && f->eax==11u) {
            f->eax=port_postef_clear_mask(&sched_postef,
                task_pcb[sched_current].pid,2,f->ebx)==0 ? 0u : (uint32_t)-1;
            return 0;
        }
        if (sched_active && sched_event_test && (f->eax==12u || f->eax==13u)) {
            int all=f->eax==13u;
            if (!f->ebx) {f->eax=(uint32_t)-1; return 0;}
            if (port_event_satisfied(&common_events,f->ebx,all)) {
                int rc=port_postef_wait(&sched_postef,
                    task_pcb[sched_current].pid,2,f->ebx,all);
                if (rc==PORT_POSTEF_WAIT_IMMEDIATE) ++event_immediate;
                f->eax=rc==PORT_POSTEF_WAIT_IMMEDIATE ? 0u : (uint32_t)-1;
                return 0;
            }
            unsigned next=sched_current^1u;
            if (task_pcb[next].state!=PORT_PCB_READY &&
                task_pcb[next].state!=PORT_PCB_RUNNING) {
                f->eax=(uint32_t)-2; /* avoid blocking the final runner */
                return 0;
            }
            if (port_postef_wait(&sched_postef,
                    task_pcb[sched_current].pid,2,f->ebx,all)!=PORT_POSTEF_WAIT_BLOCKED) {
                f->eax=(uint32_t)-1;
                return 0;
            }
            ++event_blocks;
            f->eax=0;  /* success when this task eventually resumes */
            schedule_next(f,0);
            return 0;
        }
        if (sched_active && sched_event_test && f->eax==15u) {
            task_pcb[sched_current].memory_report=f->ebx;
            f->eax=0;
            return 0;
        }
        if (sched_active && sched_demand_test && f->eax==8u) {
            task_pcb[sched_current].reports=f->ebx; /* initial zero check */
            f->eax=0;
            return 0;
        }
        if (sched_active && sched_demand_test && f->eax==9u) {
            task_pcb[sched_current].memory_report2=f->ebx;
            f->eax=0;
            return 0;
        }
        if (sched_active && (sched_vm_test || sched_demand_test) && f->eax==7u) {
            task_pcb[sched_current].memory_report=f->ebx;
            f->eax=0;
            return 0;
        }
        if (sched_active && f->eax==3u) {
            schedule_next(f,0);
            ast_on_user_return(f);
            return 0;
        }
        if (sched_active && f->eax==4u) {
            if (f->ebx != sched_current+1u || task_pcb[sched_current].reports>=3u) {
                arch_report_fault(f->vector,f->error,f->eip);
                for (;;) __asm__ volatile ("cli; hlt");
            }
            ++sched_reports; ++task_pcb[sched_current].reports;
            sched_sequence=(sched_sequence<<4)|(sched_current+1u);
            f->eax=0;
            ast_on_user_return(f);
            return 0;
        }
        if (f->eax==0u) {
            ++user_exits;
            if (sched_active) {
                schedule_next(f,1);
                if (sched_active) ast_on_user_return(f);
                return sched_active ? 0 : 1;
            }
            return 1;  /* arch_isr_common resumes the saved CPL0 frame */
        }
        arch_report_fault(f->vector,f->error,f->eip);
        for (;;) __asm__ volatile ("cli; hlt");
    }
    if (f->vector == 32) {
        ++ticks;
        /* The ISR executes the registered native SYS.EXE timer routine.
         * Never advance the pre-INIT clock after ownership transfer. */
        if(resident_exe_tick)resident_exe_tick();
        else if(exe_clock_valid)port_exe_systime_tick(&exe_system_clock);
        if (sched_active && (f->cs&3u)==3u)
            port_pcb_tick(&task_pcb[sched_current]);
        /* Identity alias 0x400000 is supervisor-only and RW, whereas the
         * same physical tick page is user-visible RO under CR0.WP=1. */
        *(volatile uint32_t *)0x400000u=ticks;
        outb(0x20, 0x20);  /* EOI to master PIC */
        if(resident_vms_sched_interrupt){
            const int result=arch_vms_schedule_frame(f);
            if(result==ARCH_TRAP_RESTORE_SELECTED_FRAME)return result;
            /* Contextless source PCBs remain queued; no idle frame is made. */
        }
        if (sched_active && sched_preempt && (f->cs&3u)==3u) {
            unsigned from=sched_current;
            schedule_next(f,0);
            if (sched_current!=from) ++sched_preemptions;
            ast_on_user_return(f);
        }
        return 0;
    } else if (f->vector == 33) {
        /* IRQ1 is masked. Ack a stray IRQ if received. */
        outb(0x20, 0x20);
        return 0;
    } else if (f->vector == 46) {
        /* Primary ATA channel completion: drain device status before EOI. */
        struct port_dq_crb *crb=disk_irq_crb;
        if(crb){
            if(port_dq_crb_interrupt(crb,f->vector)!=PORT_DQ_CRB_OK)
                ++disk_irq_bind_faults;
        }else if(disk_irq_handler)disk_irq_handler();
        outb(0xa0,0x20);
        outb(0x20,0x20);
        return 0;
    } else {
        /* Real i386 CPU exceptions enter here. Only two explicitly armed
         * diagnostic sites may resume; every other unexpected fault halts. */
        int expected_break = f->vector==3 &&
                             f->eip==(uint32_t)(uintptr_t)&arch_probe_int3_after;
        int expected_badop = f->vector==6 &&
                             f->eip==(uint32_t)(uintptr_t)&arch_probe_ud2;
        int expected_userfault = f->vector==14 &&
                   f->eip==(uint32_t)(uintptr_t)arch_user_fault_site &&
                   (f->cs&3u)==3u && (f->error&7u)==7u && user_test_active;
        int expected_isolation = f->vector==14 && isolation_active &&
                    f->eip==(uint32_t)(uintptr_t)arch_user_iso_fault_site &&
                    (f->cs&3u)==3u && (f->error&7u)==7u;
        uint32_t fault_va=0;
        if (f->vector==14) {
            __asm__ volatile ("movl %%cr2,%0" : "=r"(fault_va));
        }
        if (f->vector==14 && (f->cs&3u)==3u &&
            sched_active && sched_demand_test && user_test_active) {
            int handled=port_vmem_fault(&task_demand[sched_current],
                        &physical_pool,task_page_table[sched_current],
                        fault_va,f->error,zero_physical_frame,0);
            if (handled==PORT_VMEM_HANDLED) {
                /* Invalidate only the faulting virtual address in the
                 * current CR3; retry the exact faulting user instruction. */
                __asm__ volatile("invlpg (%0)" : : "r"(fault_va) : "memory");
                return 0;
            }
        }
        expected_userfault = expected_userfault && fault_va==0x10000u;
        expected_isolation = expected_isolation &&
                             fault_va==(uint32_t)(uintptr_t)task_b_stack;
        struct port_condition condition = {
            .type=port_condition_from_x86(f->vector),
            .vector=f->vector, .error=f->error, .eip=f->eip,
            .cs=f->cs, .eflags=f->eflags, .fault_va=fault_va,
            .mode=(uint8_t)(f->cs & 3u),
            .continuable=(uint8_t)(expected_break || expected_badop ||
                expected_userfault || expected_isolation)
        };
        enum port_condition_action action=
            port_condition_raise(&portlab_conditions,&condition);
        if (action==PORT_COND_CONTINUE && expected_break) return 0;
        if (action==PORT_COND_CONTINUE && expected_badop) {
            f->eip+=2; /* UD2 is exactly two bytes, at the known test site. */
            return 0;
        }
        if (action==PORT_COND_CONTINUE && expected_isolation) {
            f->eip=(uint32_t)(uintptr_t)arch_user_iso_fault_after;
            ++isolation_faults;
            return 0;
        }
        if (action==PORT_COND_CONTINUE && expected_userfault) {
            f->eip=(uint32_t)(uintptr_t)arch_user_fault_after;
            ++user_faults;
            return 0;
        }
        arch_report_fault(f->vector, f->error, f->eip);
        for (;;) __asm__ volatile ("cli; hlt");
    }
}
uint32_t arch_ticks(void) {
    /* aligned 32-bit loads are atomic on IA-32 */
    return ticks;
}
uint32_t arch_last_vector(void) { return last_vector; }

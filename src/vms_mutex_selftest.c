#include "vms_mutex.h"
static uint32_t pd[1024] __attribute__((aligned(4096)));
uint32_t port_mutex_selftest(void) {
    static struct port_mutex_service s;
    static struct port_pcb a,b,c,d;
    struct port_mutex m,n;
    uint32_t err=0;
    port_mutex_init(&s);port_mutex_init_one(&m);port_mutex_init_one(&n);
    if(port_pcb_create(&a,0x5201,pd,0x24000,0x10000) ||
       port_pcb_create(&b,0x5202,pd,0x24000,0x10000) ||
       port_pcb_create(&c,0x5203,pd,0x24000,0x10000) ||
       port_pcb_create(&d,0x5204,pd,0x24000,0x10000) ||
       port_pcb_activate(&a)||port_pcb_activate(&b)||
       port_pcb_activate(&c)||port_pcb_activate(&d) ||
       port_mutex_register(&s,&a,4,3) || port_mutex_register(&s,&b,16,16) ||
       port_mutex_register(&s,&c,8,7) || port_mutex_register(&s,&d,9,9))
       return 0x80000000u;
    if(port_mutex_lockr(&s,a.pid,&m) || port_mutex_lockr(&s,b.pid,&m) ||
       m.count!=1 || s.procs[0].current_priority!=15 ||
       s.procs[0].saved_priority!=4 || s.procs[1].current_priority!=16 ||
       port_mutex_lockwnowait(&s,c.pid,&m)!=PORT_MUTEX_BUSY ||
       !m.write_pending)err|=1u;
    if(port_mutex_lockw(&s,c.pid,&m)!=PORT_MUTEX_BLOCKED ||
       c.state!=PORT_PCB_WAIT_RES || !m.write_pending ||
       port_mutex_lockr(&s,a.pid,&n) ||
       port_mutex_lockr(&s,d.pid,&m)!=PORT_MUTEX_BLOCKED ||
       d.state!=PORT_PCB_WAIT_RES)err|=2u;
    if(s.procs[0].total_held!=2 || port_mutex_unlock(&s,b.pid,&m) ||
       m.count!=0 || port_mutex_waiting(&s)!=2 ||
       port_mutex_ravail(&s,31)!=0)err|=4u;
    if(port_mutex_unlock(&s,a.pid,&n) || s.procs[0].current_priority!=15 ||
       port_mutex_unlock(&s,a.pid,&m) || m.count!=-1 ||m.write_pending ||
       c.state!=PORT_PCB_READY || d.state!=PORT_PCB_READY ||
       s.procs[0].current_priority!=4 || s.procs[0].base_priority!=3)
       err|=8u;
    if(port_pcb_activate(&c) || port_mutex_lockw(&s,c.pid,&m) ||
       m.count!=0 || !m.write_pending || port_pcb_activate(&d) ||
       port_mutex_lockr(&s,d.pid,&m)!=PORT_MUTEX_BLOCKED ||
       port_mutex_unlock(&s,c.pid,&m) || m.count!=-1 ||
       d.state!=PORT_PCB_READY || port_pcb_activate(&d) ||
       port_mutex_lockr(&s,d.pid,&m) || port_mutex_unlock(&s,d.pid,&m))
       err|=16u;
    if(port_mutex_iolockr(&s,b.pid) ||
       port_mutex_iolockw(&s,c.pid)!=PORT_MUTEX_BLOCKED ||
       port_mutex_iounlock(&s,b.pid) ||
       c.state!=PORT_PCB_READY || port_pcb_activate(&c) ||
       port_mutex_iolockw(&s,c.pid) || port_mutex_iounlock(&s,c.pid))err|=32u;
    if(port_mutex_rwait(&s,c.pid,31)!=PORT_MUTEX_BLOCKED ||
       !(s.resource_mask&(1u<<31)) || port_mutex_ravail(&s,31)!=1 ||
       c.state!=PORT_PCB_READY || s.resource_mask ||
       port_mutex_ravail(&s,31)!=0 || port_pcb_activate(&c))err|=64u;
    s.highest_ready_priority=20;
    if(port_mutex_lockr(&s,c.pid,&n) || port_mutex_unlock(&s,c.pid,&n) ||
       !s.preempt_pending || s.procs[2].current_priority!=8 ||
       s.procs[2].base_priority!=7)err|=128u;
    if(port_mutex_unlock(&s,c.pid,&n)!=PORT_MUTEX_NOT_OWNER ||
       port_mutex_lockr(&s,0xDEAD,&n)!=PORT_MUTEX_BAD_PROCESS ||
       port_mutex_rwait(&s,c.pid,32)!=PORT_MUTEX_BAD_RESOURCE ||
       port_mutex_register(&s,&a,3,4)!=PORT_MUTEX_BAD_PROCESS ||
       port_mutex_unregister(&s,c.pid) || port_mutex_unregister(&s,b.pid) ||
       port_mutex_unregister(&s,d.pid) ||port_mutex_unregister(&s,a.pid))err|=256u;
    if(s.waits!=5 || s.wakeups!=5 || port_mutex_waiting(&s))err|=512u;
    return err;
}

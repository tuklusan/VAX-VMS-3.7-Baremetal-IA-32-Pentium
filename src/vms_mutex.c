#include "vms_mutex.h"

static struct port_mutex_process *lookup(struct port_mutex_service *s,uint32_t pid) {
    if(!s || !pid)return 0;
    for(unsigned i=0;i<PORT_MUTEX_PROCS;i++)
        if(s->procs[i].pcb && s->procs[i].pcb->pid==pid)return &s->procs[i];
    return 0;
}
void port_mutex_init_one(struct port_mutex *m) {
    if(!m)return;
    m->count=-1;m->write_pending=0;
}
void port_mutex_init(struct port_mutex_service *s) {
    if(!s)return;
    port_mutex_init_one(&s->io_database);
    s->resource_mask=s->sequence=s->wakeups=s->waits=s->preempt_pending=0;
    s->highest_ready_priority=0;
    for(unsigned i=0;i<PORT_MUTEX_PROCS;i++){
        struct port_mutex_process *p=&s->procs[i];
        p->pcb=0;p->total_held=0;
        p->current_priority=p->base_priority=p->saved_priority=p->saved_base=0;
        p->waiting=0;p->wait_mutex=0;p->wait_resource=p->wait_order=p->wakes=0;
        for(unsigned j=0;j<PORT_MUTEX_PER_PROC;j++){
            p->holds[j].mutex=0;p->holds[j].count=0;p->holds[j].exclusive=0;
        }
    }
}
int port_mutex_register(struct port_mutex_service *s,struct port_pcb *pcb,
                        unsigned current_priority,unsigned base_priority) {
    if(!s || !pcb || !pcb->pid || current_priority>31 || base_priority>31 ||
       lookup(s,pcb->pid))return PORT_MUTEX_BAD_PROCESS;
    for(unsigned i=0;i<PORT_MUTEX_PROCS;i++)if(!s->procs[i].pcb){
        struct port_mutex_process *p=&s->procs[i];
        p->pcb=pcb;
        p->current_priority=(uint8_t)current_priority;
        p->base_priority=(uint8_t)base_priority;
        p->saved_priority=p->saved_base=0;
        p->total_held=p->waiting=0;p->wakes=0;
        p->wait_mutex=0;p->wait_resource=p->wait_order=0;
        for(unsigned j=0;j<PORT_MUTEX_PER_PROC;j++){
            p->holds[j].mutex=0;p->holds[j].count=0;p->holds[j].exclusive=0;
        }
        return 0;
    }
    return PORT_MUTEX_LIMIT;
}
int port_mutex_unregister(struct port_mutex_service *s,uint32_t pid) {
    struct port_mutex_process *p=lookup(s,pid);
    if(!p)return PORT_MUTEX_BAD_PROCESS;
    if(p->total_held || p->waiting)return PORT_MUTEX_BAD_STATE;
    p->pcb=0;
    return 0;
}
static struct port_mutex_hold *existing_hold(struct port_mutex_process *p,
                                             struct port_mutex *m) {
    for(unsigned i=0;i<PORT_MUTEX_PER_PROC;i++)
        if(p->holds[i].mutex==m)return &p->holds[i];
    return 0;
}
static struct port_mutex_hold *empty_hold(struct port_mutex_process *p) {
    for(unsigned i=0;i<PORT_MUTEX_PER_PROC;i++)
        if(!p->holds[i].mutex)return &p->holds[i];
    return 0;
}
static int acquire(struct port_mutex_process *p,struct port_mutex *m,int exclusive) {
    struct port_mutex_hold *h=existing_hold(p,m);
    if(h && (h->exclusive || exclusive || h->count==65535))return PORT_MUTEX_BAD_STATE;
    if(!h)h=empty_hold(p);
    if(!h || p->total_held==65535)return PORT_MUTEX_LIMIT;
    if(!p->total_held){
        p->saved_priority=p->current_priority;
        p->saved_base=p->base_priority;
        if(p->current_priority<16){
            p->current_priority=15;p->base_priority=15;
        }
    }
    if(!h->mutex){h->mutex=m;h->count=0;h->exclusive=(uint8_t)exclusive;}
    ++h->count;++p->total_held;
    ++m->count;
    return PORT_MUTEX_ACQUIRED;
}
static int wait_for(struct port_mutex_service *s,struct port_mutex_process *p,
                    struct port_mutex *m,unsigned resource) {
    if(p->waiting || port_pcb_block_res(p->pcb))return PORT_MUTEX_BAD_STATE;
    p->waiting=(uint8_t)(m?1u:2u);
    p->wait_mutex=m;p->wait_resource=resource;
    p->wait_order=++s->sequence;
    ++s->waits;
    return PORT_MUTEX_BLOCKED;
}
int port_mutex_lockr(struct port_mutex_service *s,uint32_t pid,struct port_mutex *m) {
    struct port_mutex_process *p=lookup(s,pid);
    if(!p)return PORT_MUTEX_BAD_PROCESS;
    if(!m || p->pcb->state!=PORT_PCB_RUNNING || p->waiting)return PORT_MUTEX_BAD_STATE;
    if(m->write_pending)return wait_for(s,p,m,0);
    if(m->count>=32766)return PORT_MUTEX_LIMIT;
    return acquire(p,m,0);
}
static int write_lock(struct port_mutex_service *s,uint32_t pid,
                      struct port_mutex *m,int nowait) {
    struct port_mutex_process *p=lookup(s,pid);
    if(!p)return PORT_MUTEX_BAD_PROCESS;
    if(!m || p->pcb->state!=PORT_PCB_RUNNING || p->waiting)return PORT_MUTEX_BAD_STATE;
    /* A waiting writer holds the pending bit until the last current owner exits. */
    if(m->write_pending || m->count!=-1){
        if(nowait){
            /* The original nonwaiting writer attempt leaves the pending
             * bit set until the last existing owner releases the mutex. */
            m->write_pending=1;
            return PORT_MUTEX_BUSY;
        }
        int blocked=wait_for(s,p,m,0);
        if(blocked==PORT_MUTEX_BLOCKED)m->write_pending=1;
        return blocked;
    }
    if(!empty_hold(p) || p->total_held==65535)return PORT_MUTEX_LIMIT;
    m->write_pending=1;
    int rc=acquire(p,m,1);
    if(rc!=PORT_MUTEX_ACQUIRED)m->write_pending=0;
    return rc;
}
int port_mutex_lockw(struct port_mutex_service *s,uint32_t pid,struct port_mutex *m) {
    return write_lock(s,pid,m,0);
}
int port_mutex_lockwnowait(struct port_mutex_service *s,uint32_t pid,struct port_mutex *m) {
    return write_lock(s,pid,m,1);
}
static int matches(const struct port_mutex_process *p,struct port_mutex *m,
                   unsigned resource) {
    return p->waiting && (m ? (p->waiting==1 && p->wait_mutex==m) :
                            (p->waiting==2 && p->wait_resource==resource));
}
static unsigned wake_matching(struct port_mutex_service *s,struct port_mutex *m,
                              unsigned resource) {
    unsigned woke=0;
    /* Each waiting PCB occupies one queue position. The queue scan is
     * ordered by wait sequence to match the original rescheduling order. */
    for(unsigned n=0;n<PORT_MUTEX_PROCS;n++){
        struct port_mutex_process *best=0;
        for(unsigned i=0;i<PORT_MUTEX_PROCS;i++){
            struct port_mutex_process *p=&s->procs[i];
            if(matches(p,m,resource) && (!best || p->wait_order<best->wait_order))
                best=p;
        }
        if(!best)break;
        if(port_pcb_wake_res(best->pcb))break;
        best->waiting=0;best->wait_mutex=0;best->wait_resource=best->wait_order=0;
        ++best->wakes;++s->wakeups;++woke;
    }
    return woke;
}
int port_mutex_unlock(struct port_mutex_service *s,uint32_t pid,struct port_mutex *m) {
    struct port_mutex_process *p=lookup(s,pid);
    if(!p)return PORT_MUTEX_BAD_PROCESS;
    if(!m || p->pcb->state!=PORT_PCB_RUNNING || p->waiting)return PORT_MUTEX_BAD_STATE;
    struct port_mutex_hold *h=existing_hold(p,m);
    if(!h || !h->count || !p->total_held || m->count<0)return PORT_MUTEX_NOT_OWNER;
    --h->count;--p->total_held;--m->count;
    if(!h->count){h->mutex=0;h->exclusive=0;}
    if(!p->total_held){
        p->current_priority=p->saved_priority;
        p->base_priority=p->saved_base;
        if(s->highest_ready_priority>p->current_priority)s->preempt_pending=1;
    }
    if(m->count<0){
        m->write_pending=0;
        (void)wake_matching(s,m,0);
    }
    return PORT_MUTEX_ACQUIRED;
}
int port_mutex_iolockr(struct port_mutex_service *s,uint32_t pid) {
    return s?port_mutex_lockr(s,pid,&s->io_database):PORT_MUTEX_BAD_STATE;
}
int port_mutex_iolockw(struct port_mutex_service *s,uint32_t pid) {
    return s?port_mutex_lockw(s,pid,&s->io_database):PORT_MUTEX_BAD_STATE;
}
int port_mutex_iounlock(struct port_mutex_service *s,uint32_t pid) {
    return s?port_mutex_unlock(s,pid,&s->io_database):PORT_MUTEX_BAD_STATE;
}
int port_mutex_rwait(struct port_mutex_service *s,uint32_t pid,unsigned resource) {
    struct port_mutex_process *p=lookup(s,pid);
    if(!p)return PORT_MUTEX_BAD_PROCESS;
    if(resource>=PORT_MUTEX_RESOURCES)return PORT_MUTEX_BAD_RESOURCE;
    if(p->pcb->state!=PORT_PCB_RUNNING || p->waiting)return PORT_MUTEX_BAD_STATE;
    int blocked=wait_for(s,p,0,resource);
    if(blocked==PORT_MUTEX_BLOCKED)s->resource_mask|=1u<<resource;
    return blocked;
}
int port_mutex_ravail(struct port_mutex_service *s,unsigned resource) {
    if(!s || resource>=PORT_MUTEX_RESOURCES)return PORT_MUTEX_BAD_RESOURCE;
    uint32_t bit=1u<<resource;
    if(!(s->resource_mask&bit))return 0;
    s->resource_mask&=~bit;
    return (int)wake_matching(s,0,resource);
}
uint32_t port_mutex_waiting(const struct port_mutex_service *s) {
    unsigned n=0;
    if(s)for(unsigned i=0;i<PORT_MUTEX_PROCS;i++)n+=s->procs[i].waiting!=0;
    return n;
}

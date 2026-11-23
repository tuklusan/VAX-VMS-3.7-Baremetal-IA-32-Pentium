#include "vms_exe_schevt.h"
#include <stddef.h>

/* Time queue ownership is protected by the caller's scheduler lock. */
void port_exe_timer_init(struct port_exe_timer_queue *queue,uint64_t now) {
    if(!queue)return;
    queue->now=now;
    queue->used=queue->delivered=queue->failed=0;
    for(unsigned i=0;i<PORT_EXE_TQE_CAPACITY;i++)
        queue->entries[i].kind=0;
}
static uint32_t fetch_quad(const struct port_exe_timer_ops *ops,
                           uint32_t address,uint64_t *value) {
    if(!ops || !ops->read_user || !address ||
       !ops->read_user(ops->context,address,value,8u))return PORT_SS_ACCVIO;
    return PORT_SS_NORMAL;
}
static uint32_t expiration(uint64_t now,uint64_t raw,uint64_t *when,
                           uint8_t *absolute) {
    *absolute=((raw>>63)==0u);
    if(*absolute) {
        *when=raw;
    } else {
        uint64_t duration=~raw+UINT64_C(1);
        if(UINT64_MAX-now<duration)return PORT_SS_IVTIME;
        *when=now+duration;
    }
    return PORT_SS_NORMAL;
}
static struct port_exe_timer_entry *free_entry(struct port_exe_timer_queue *queue) {
    if(!queue || queue->used>=PORT_EXE_TQE_CAPACITY)return NULL;
    for(unsigned i=0;i<PORT_EXE_TQE_CAPACITY;i++)
        if(!queue->entries[i].kind)return &queue->entries[i];
    return NULL;
}
static struct port_exe_timer_entry *allocate_entry(
    struct port_exe_timer_queue *queue,struct port_exe_timer_process *owner,
    const struct port_exe_timer_ops *ops) {
    struct port_exe_timer_entry *entry=free_entry(queue);
    if(entry || owner->implicit_wait_disabled || !ops->wait_resource)
        return entry;
    uint32_t status=ops->wait_resource(ops->context,owner->pid,
                                      PORT_EXE_WAIT_NONPAGED);
    return (status&1u)?free_entry(queue):NULL;
}
static void insert(struct port_exe_timer_queue *queue,
                   struct port_exe_timer_entry *entry,uint8_t kind,
                   struct port_exe_timer_process *owner,
                   uint32_t target,uint64_t when,uint8_t absolute) {
    entry->owner=owner;
    entry->requester_pid=owner->pid;
    entry->target_pid=target;
    entry->when=when;
    entry->absolute=absolute;
    entry->kind=kind;
    ++queue->used;
}
uint32_t port_exe_schdwk(struct port_exe_timer_queue *queue,
                         struct port_exe_timer_process *caller,
                         const struct port_exe_wake_request *request,
                         const struct port_exe_timer_ops *ops) {
    if(!queue || !caller || !request || !ops || !ops->wake_process)
        return PORT_SS_BADQUEUEHDR;
    uint32_t target=caller->pid;
    if(request->pid_address || request->logical_name) {
        if(!ops->resolve_process)return PORT_SS_NONEXPR;
        uint32_t status=ops->resolve_process(ops->context,request->pid_address,
                                    request->logical_name,caller->pid,&target);
        if(!(status&1u))return status;
        if(!target)return PORT_SS_NONEXPR;
    }
    uint64_t repeat=0;
    if(request->repeat_time) {
        uint64_t raw;
        uint32_t status=fetch_quad(ops,request->repeat_time,&raw);
        if(!(status&1u))return status;
        if(raw && (raw>>63)==0u)return PORT_SS_IVTIME;
        if(raw) {
            repeat=~raw+UINT64_C(1);
            if(repeat<PORT_EXE_TICK_100NS)repeat=PORT_EXE_TICK_100NS;
        }
    }
    uint64_t raw,when;
    uint8_t absolute;
    uint32_t status=fetch_quad(ops,request->day_time,&raw);
    if(!(status&1u))return status;
    status=expiration(queue->now,raw,&when,&absolute);
    if(!(status&1u))return status;
    if(repeat && when<queue->now) {
        /* A repeat interval entirely in the past is not schedulable. */
        if(UINT64_MAX-when<repeat || when+repeat<queue->now)
            return PORT_SS_IVTIME;
    }
    if(!caller->ast_available)return PORT_SS_EXQUOTA;
    struct port_exe_timer_entry *entry=allocate_entry(queue,caller,ops);
    if(!entry)return PORT_SS_INSFMEM;
    insert(queue,entry,repeat?PORT_EXE_TQE_WKREPT:PORT_EXE_TQE_WKSNGL,
           caller,target,when,absolute);
    entry->interval=repeat;
    entry->ast=entry->ast_parameter=entry->efn=entry->mode=0;
    --caller->ast_available;
    return PORT_SS_NORMAL;
}
uint32_t port_exe_setimr(struct port_exe_timer_queue *queue,
                         struct port_exe_timer_process *caller,
                         const struct port_exe_timer_request *request,
                         const struct port_exe_timer_ops *ops) {
    if(!queue || !caller || !request || !ops || !ops->post_event ||
       (request->ast_address && !ops->queue_ast))return PORT_SS_BADQUEUEHDR;
    if(!caller->events)return PORT_SS_UNASEFC;
    if(!caller->job)return PORT_SS_BADPARAM;
    /* SETIMR clears the caller's event flag before reading the time. */
    uint32_t efn=request->efn&255u;
    uint32_t status=port_exe_clref(caller->events,efn);
    if(!(status&1u))return status;
    uint64_t raw,when;
    uint8_t absolute;
    status=fetch_quad(ops,request->day_time,&raw);
    if(!(status&1u))return status;
    status=expiration(queue->now,raw,&when,&absolute);
    if(!(status&1u))return status;
    if(request->ast_address && !caller->ast_available)return PORT_SS_EXQUOTA;
    if(!caller->job->timer_available) {
        if(caller->implicit_wait_disabled || !ops->wait_resource)
            return PORT_SS_EXQUOTA;
        uint32_t result=ops->wait_resource(ops->context,caller->pid,
                                          PORT_EXE_WAIT_TIMER_QUOTA);
        if(!(result&1u) || !caller->job->timer_available)return PORT_SS_EXQUOTA;
    }
    struct port_exe_timer_entry *entry=allocate_entry(queue,caller,ops);
    if(!entry)return PORT_SS_INSFMEM;
    insert(queue,entry,PORT_EXE_TQE_TMSNGL,caller,caller->pid,when,absolute);
    entry->interval=0;
    entry->ast=request->ast_address;
    entry->ast_parameter=request->request_id;
    entry->efn=(uint8_t)efn;
    entry->mode=caller->previous_mode&3u;
    --caller->job->timer_available;
    if(entry->ast)--caller->ast_available;
    return PORT_SS_NORMAL;
}
static void restore_quota(struct port_exe_timer_entry *entry) {
    if(!entry->owner)return;
    if(entry->kind==PORT_EXE_TQE_TMSNGL) {
        ++entry->owner->job->timer_available;
        if(entry->ast)++entry->owner->ast_available;
    } else {
        ++entry->owner->ast_available;
    }
}
unsigned port_exe_timer_advance(struct port_exe_timer_queue *queue,
                               uint64_t now,const struct port_exe_timer_ops *ops) {
    if(!queue || !ops || now<queue->now)return 0;
    queue->now=now;
    unsigned fired=0;
    uint64_t visited=0;
    unsigned initial=queue->used;
    for(unsigned count=0;count<initial;count++) {
        /* EXE$INSTIMQ orders work by expiration, not free-list position. */
        struct port_exe_timer_entry *e=NULL;
        unsigned choice=0;
        for(unsigned i=0;i<PORT_EXE_TQE_CAPACITY;i++) {
            struct port_exe_timer_entry *candidate=&queue->entries[i];
            if((visited&(UINT64_C(1)<<i)) || !candidate->kind ||
               candidate->when>now)continue;
            if(!e || candidate->when<e->when) {
                e=candidate;
                choice=i;
            }
        }
        if(!e)break;
        visited|=UINT64_C(1)<<choice;
        uint32_t result;
        if(e->kind==PORT_EXE_TQE_TMSNGL) {
            result=ops->post_event?
                ops->post_event(ops->context,e->target_pid,e->efn,0u):
                PORT_SS_BADQUEUEHDR;
            if((result&1u) && e->ast)
                result=ops->queue_ast?
                    ops->queue_ast(ops->context,e->target_pid,e->ast,
                                   e->ast_parameter,e->mode):PORT_SS_BADQUEUEHDR;
        } else {
            result=ops->wake_process?
                ops->wake_process(ops->context,e->target_pid):PORT_SS_BADQUEUEHDR;
        }
        if(result&1u)++queue->delivered;
        else ++queue->failed;
        ++fired;
        if(e->kind==PORT_EXE_TQE_WKREPT && e->interval &&
           UINT64_MAX-now>=e->interval) {
            uint64_t next=e->when;
            if(UINT64_MAX-next>=e->interval)next+=e->interval;
            else next=now+e->interval;
            if(next<=now)next=now+e->interval;
            e->when=next;
        } else {
            restore_quota(e);
            e->kind=0;
            --queue->used;
        }
    }
    return fired;
}

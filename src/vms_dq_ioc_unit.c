#include "vms_dq_ioc.h"

static uint16_t get16(const uint8_t *p){
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8u));
}
static void set16(uint8_t *p,uint16_t value){
    p[0]=(uint8_t)value;p[1]=(uint8_t)(value>>8u);
}
static uint32_t enter(struct port_dq_ioc_unit *unit){
    return unit->raise_sync(unit->sync_context);
}
static void leave(struct port_dq_ioc_unit *unit,uint32_t previous){
    unit->restore_sync(unit->sync_context,previous);
}
static unsigned queued(const struct port_dq_ioc_queue *queue){
    unsigned count=0u;
    for(unsigned i=0u;i<PORT_DQ_IOC_DEPTH;i++)
        count+=queue->packets[i].state==PORT_DQ_IOC_QUEUED;
    return count;
}
static uint32_t start_next(struct port_dq_ioc_unit *unit){
    uint32_t tag=0u;
    const uint32_t status=port_dq_ioc_start_async_context(unit->queue,
        unit->start,unit->start_context,&tag);
    unit->last_start_status=status;
    if(status==PORT_DQ_IOC_OK||status==PORT_DQ_IOC_IO){
        if(unit->starts==UINT32_MAX)return PORT_DQ_IOC_CORRUPT;
        unit->starts++;
        return status;
    }
    return PORT_DQ_IOC_CORRUPT;
}

uint32_t port_dq_ioc_unit_init(struct port_dq_ioc_unit *unit,
    struct port_dq_ioc_queue *queue,uint8_t *ucb,
    port_dq_ioc_async_start_fn start,void *start_context,
    port_dq_ioc_raise_fn raise_sync,port_dq_ioc_restore_fn restore_sync,
    void *sync_context){
    if(!unit||!queue||!ucb||!start||!raise_sync||!restore_sync||
       ((uintptr_t)ucb&3u)||get16(ucb+8u)<132u||
       port_dq_ioc_validate(queue)!=PORT_DQ_IOC_OK||
       queue->active!=PORT_DQ_IOC_DEPTH||queued(queue)||
       (get16(ucb+PORT_DQ_IOC_UCB_STS_OFFSET)&PORT_DQ_IOC_UCB_BUSY))
        return PORT_DQ_IOC_BAD;
    *unit=(struct port_dq_ioc_unit){.queue=queue,.ucb=ucb,.start=start,
        .start_context=start_context,.raise_sync=raise_sync,
        .restore_sync=restore_sync,.sync_context=sync_context};
    return PORT_DQ_IOC_OK;
}

uint32_t port_dq_ioc_unit_submit(struct port_dq_ioc_unit *unit,uint32_t tag){
    if(!unit||!unit->queue||!unit->ucb||!unit->start||
       !unit->raise_sync||!unit->restore_sync)return PORT_DQ_IOC_BAD;
    const uint32_t previous=enter(unit);
    const uint16_t status=get16(unit->ucb+PORT_DQ_IOC_UCB_STS_OFFSET);
    const int busy=(status&PORT_DQ_IOC_UCB_BUSY)!=0u;
    if((busy&&unit->queue->active>=PORT_DQ_IOC_DEPTH)||
       (!busy&&unit->queue->active<PORT_DQ_IOC_DEPTH)){
        leave(unit,previous);return PORT_DQ_IOC_CORRUPT;
    }
    const uint32_t enqueued=port_dq_ioc_enqueue(unit->queue,tag);
    if(enqueued!=PORT_DQ_IOC_OK){leave(unit,previous);return enqueued;}
    if(busy){leave(unit,previous);return PORT_DQ_IOC_DEFERRED;}
    set16(unit->ucb+PORT_DQ_IOC_UCB_STS_OFFSET,
        (uint16_t)(status|PORT_DQ_IOC_UCB_BUSY));
    const uint32_t started=start_next(unit);
    leave(unit,previous);
    return started;
}

uint32_t port_dq_ioc_unit_post(struct port_dq_ioc_unit *unit,
    struct port_dq_ioc_iosb *iosb){
    if(!unit||!unit->queue||!unit->ucb||!iosb||!unit->raise_sync||
       !unit->restore_sync)return PORT_DQ_IOC_BAD;
    const uint32_t previous=enter(unit);
    const uint16_t status=get16(unit->ucb+PORT_DQ_IOC_UCB_STS_OFFSET);
    if(!(status&PORT_DQ_IOC_UCB_BUSY)||
       unit->queue->active>=PORT_DQ_IOC_DEPTH||unit->posted==UINT32_MAX){
        leave(unit,previous);return PORT_DQ_IOC_CORRUPT;
    }
    const uint32_t posted=port_dq_ioc_post(unit->queue,iosb);
    if(posted!=PORT_DQ_IOC_OK&&posted!=PORT_DQ_IOC_IO){
        leave(unit,previous);return posted;
    }
    unit->posted++;
    if(queued(unit->queue)){
        if(start_next(unit)==PORT_DQ_IOC_CORRUPT){
            leave(unit,previous);return PORT_DQ_IOC_CORRUPT;
        }
    }else{
        set16(unit->ucb+PORT_DQ_IOC_UCB_STS_OFFSET,
            (uint16_t)(status&~PORT_DQ_IOC_UCB_BUSY));
    }
    if(port_dq_ioc_validate(unit->queue)!=PORT_DQ_IOC_OK){
        leave(unit,previous);return PORT_DQ_IOC_CORRUPT;
    }
    leave(unit,previous);
    return posted;
}

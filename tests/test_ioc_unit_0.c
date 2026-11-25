#include "dq_ioc_test_common.h"

struct unit_fixture {
    port_dq_ioc_async_done_fn done;
    void *done_context;
    uint16_t *sector;
    uint32_t starts,lbn;
    int reject;
};
static int unit_start(void *v,uint32_t lbn,uint16_t sector[256],
    port_dq_ioc_async_done_fn done,void *done_context){
    struct unit_fixture *f=v;
    f->starts++;f->lbn=lbn;f->sector=sector;
    if(f->reject)return f->reject;
    f->done=done;f->done_context=done_context;return 0;
}
struct unit_sync {uint32_t level,raises,restores;};
static uint16_t unit_status(const uint8_t *ucb){
    return (uint16_t)((uint16_t)ucb[PORT_DQ_IOC_UCB_STS_OFFSET]|
        ((uint16_t)ucb[PORT_DQ_IOC_UCB_STS_OFFSET+1u]<<8u));
}
static uint32_t unit_raise(void *v){
    struct unit_sync *s=v;uint32_t old=s->level;s->level=4u;s->raises++;return old;
}
static void unit_restore(void *v,uint32_t old){
    struct unit_sync *s=v;s->level=old;s->restores++;
}

int test_ioc_unit_0(void){
    struct port_dq_ioc_queue queue;
    struct dq_ioc_fixture read_fixture={0};
    struct unit_fixture fixture={0};
    struct unit_sync sync={0};
    _Alignas(4) uint8_t ucb[252]={0};
    uint8_t first[512]={0},second[512]={0},third[512]={0};
    ucb[8]=252u;
    assert(port_dq_ioc_init(&queue,dq_ioc_test_read,&read_fixture)==
        PORT_DQ_IOC_OK);
    struct port_dq_ioc_unit unit;
    assert(port_dq_ioc_unit_init(&unit,&queue,ucb,unit_start,&fixture,
        unit_raise,unit_restore,&sync)==PORT_DQ_IOC_OK);
    assert(port_dq_ioc_prepare(&queue,1u,7u,first,sizeof first)==
        PORT_DQ_IOC_OK);
    assert(port_dq_ioc_set_priority(&queue,1u,1u)==PORT_DQ_IOC_OK);
    assert(port_dq_ioc_prepare(&queue,2u,9u,second,sizeof second)==
        PORT_DQ_IOC_OK);
    assert(port_dq_ioc_set_priority(&queue,2u,3u)==PORT_DQ_IOC_OK);
    assert(port_dq_ioc_unit_submit(&unit,1u)==PORT_DQ_IOC_OK);
    assert((unit_status(ucb)&PORT_DQ_IOC_UCB_BUSY)!=0u);
    assert(fixture.starts==1u&&fixture.lbn==7u&&queue.active<PORT_DQ_IOC_DEPTH);
    assert(port_dq_ioc_unit_submit(&unit,2u)==PORT_DQ_IOC_DEFERRED);
    assert(fixture.starts==1u&&sync.level==0u);
    struct port_dq_ioc_iosb iosb={0};
    for(unsigned i=0;i<256u;i++)fixture.sector[i]=(uint16_t)(0x1100u+i);
    fixture.done(fixture.done_context,PORT_DQ_IOC_OK);
    assert(port_dq_ioc_unit_post(&unit,&iosb)==PORT_DQ_IOC_OK);
    assert(iosb.tag==1u&&iosb.status==PORT_DQ_IOC_OK&&iosb.bytes==512u);
    assert(fixture.starts==2u&&fixture.lbn==9u&&
        (unit_status(ucb)&PORT_DQ_IOC_UCB_BUSY)!=0u);
    assert(queue.active<PORT_DQ_IOC_DEPTH&&
        queue.packets[queue.active].tag==2u);
    for(unsigned i=0;i<256u;i++)fixture.sector[i]=(uint16_t)(0x2200u+i);
    fixture.done(fixture.done_context,PORT_DQ_IOC_OK);
    assert(port_dq_ioc_unit_post(&unit,&iosb)==PORT_DQ_IOC_OK);
    assert(iosb.tag==2u&&iosb.status==PORT_DQ_IOC_OK&&iosb.bytes==512u);
    assert(second[0]==0u&&second[1]==0x22u&&second[510]==0xffu&&
        second[511]==0x22u);
    assert((unit_status(ucb)&PORT_DQ_IOC_UCB_BUSY)==0u&&
        unit.starts==2u&&unit.posted==2u);

    fixture.reject=-1;
    assert(port_dq_ioc_prepare(&queue,3u,11u,third,sizeof third)==
        PORT_DQ_IOC_OK);
    assert(port_dq_ioc_unit_submit(&unit,3u)==PORT_DQ_IOC_IO);
    assert((unit_status(ucb)&PORT_DQ_IOC_UCB_BUSY)!=0u);
    assert(port_dq_ioc_unit_post(&unit,&iosb)==PORT_DQ_IOC_IO);
    assert(iosb.tag==3u&&iosb.status==PORT_DQ_IOC_IO&&iosb.bytes==0u);
    assert((unit_status(ucb)&PORT_DQ_IOC_UCB_BUSY)==0u);
    assert(queue.completed==2u&&queue.failed==1u&&unit.posted==3u);
    assert(port_dq_ioc_validate(&queue)==PORT_DQ_IOC_OK);
    assert(sync.level==0u&&sync.raises==sync.restores);
    return 1;
}

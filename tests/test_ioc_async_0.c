#include "dq_ioc_test_common.h"

struct async_fixture {
    struct dq_ioc_fixture read_fixture;
    port_dq_ioc_async_done_fn done;
    void *done_context;
    uint16_t *sector;
    uint32_t starts;
    int reject;
};
static int async_sector_start(void *context,uint32_t lbn,uint16_t sector[256],
    port_dq_ioc_async_done_fn done,void *done_context){
    struct async_fixture *fixture=context;
    (void)lbn;
    fixture->starts++;
    fixture->sector=sector;
    fixture->done=done;
    fixture->done_context=done_context;
    return fixture->reject;
}

int test_ioc_async_0(void){
    struct port_dq_ioc_queue q;
    struct async_fixture fixture={0};
    uint8_t first[512],second[512];
    memset(first,0xee,sizeof first);memset(second,0xee,sizeof second);
    assert(port_dq_ioc_init(&q,dq_ioc_test_read,&fixture.read_fixture)==
        PORT_DQ_IOC_OK);
    assert(port_dq_ioc_prepare(&q,1u,4u,first,sizeof first)==PORT_DQ_IOC_OK);
    assert(port_dq_ioc_set_priority(&q,1u,1u)==PORT_DQ_IOC_OK);
    assert(port_dq_ioc_enqueue(&q,1u)==PORT_DQ_IOC_OK);
    assert(port_dq_ioc_prepare(&q,2u,7u,second,sizeof second)==PORT_DQ_IOC_OK);
    assert(port_dq_ioc_set_priority(&q,2u,3u)==PORT_DQ_IOC_OK);
    assert(port_dq_ioc_enqueue(&q,2u)==PORT_DQ_IOC_OK);
    uint32_t tag=0u;
    assert(port_dq_ioc_start_async(&q,async_sector_start,&tag)==PORT_DQ_IOC_OK);
    assert(tag==1u&&fixture.starts==1u&&q.active<PORT_DQ_IOC_DEPTH);
    assert(port_dq_ioc_start_async(&q,async_sector_start,&tag)==
        PORT_DQ_IOC_BUSY&&fixture.starts==1u);
    for(unsigned i=0u;i<512u;i++)assert(first[i]==0xeeu);
    for(unsigned i=0u;i<256u;i++)fixture.sector[i]=(uint16_t)(0x5000u+i);
    fixture.done(fixture.done_context,PORT_DQ_IOC_OK);
    assert(port_dq_ioc_async_complete(&q,PORT_DQ_IOC_OK)==PORT_DQ_IOC_BUSY);
    struct port_dq_ioc_iosb iosb={0};
    assert(port_dq_ioc_post(&q,&iosb)==PORT_DQ_IOC_OK);
    assert(iosb.tag==1u&&iosb.status==PORT_DQ_IOC_OK&&iosb.bytes==512u);
    assert(first[0]==0u&&first[1]==0x50u&&first[510]==0xffu&&
           first[511]==0x50u);
    assert(port_dq_ioc_validate(&q)==PORT_DQ_IOC_OK);
    assert(port_dq_ioc_start_async(&q,async_sector_start,&tag)==PORT_DQ_IOC_OK);
    assert(tag==2u&&fixture.starts==2u);
    for(unsigned i=0u;i<256u;i++)fixture.sector[i]=(uint16_t)(0x6000u+i);
    fixture.done(fixture.done_context,PORT_DQ_IOC_OK);
    assert(port_dq_ioc_post(&q,&iosb)==PORT_DQ_IOC_OK);
    assert(iosb.tag==2u&&iosb.status==PORT_DQ_IOC_OK&&iosb.bytes==512u);
    assert(second[0]==0u&&second[1]==0x60u&&second[510]==0xffu&&
           second[511]==0x60u);
    fixture.reject=-1;
    assert(port_dq_ioc_prepare(&q,3u,9u,first,sizeof first)==PORT_DQ_IOC_OK);
    assert(port_dq_ioc_enqueue(&q,3u)==PORT_DQ_IOC_OK);
    assert(port_dq_ioc_start_async(&q,async_sector_start,&tag)==PORT_DQ_IOC_IO);
    assert(tag==3u&&fixture.starts==3u);
    assert(port_dq_ioc_post(&q,&iosb)==PORT_DQ_IOC_IO);
    assert(iosb.tag==3u&&iosb.status==PORT_DQ_IOC_IO&&iosb.bytes==0u);
    assert(port_dq_ioc_validate(&q)==PORT_DQ_IOC_OK);
    return 1;
}

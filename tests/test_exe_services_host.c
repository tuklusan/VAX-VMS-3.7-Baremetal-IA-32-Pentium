#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_exe_gettim.h"
#include "vms_exe_setssf.h"

static uint32_t pd[1024],pt[1024];
static uint8_t first[4096],second[4096];
static const uint32_t *table(void *ctx,uint32_t phys) {
    (void)ctx;return phys==0x5000u?pt:0;
}
static uint8_t *page(void *ctx,uint32_t phys) {
    (void)ctx;return phys==0x8000u?first:phys==0x9000u?second:0;
}
struct test_writer {
    struct port_exe_systime *clock;
    unsigned calls;
    uint32_t recorded[2];
};
static int write_snapshot(void *ctx,uint32_t address,const uint32_t value[2]) {
    struct test_writer *s=(struct test_writer *)ctx;
    assert(address==0x42u);
    s->recorded[0]=value[0];s->recorded[1]=value[1];
    if(++s->calls==1u)
        port_exe_systime_store(s->clock,UINT64_C(0x010203047fffffff));
    return 1;
}
static int write_denied(void *ctx,uint32_t address,const uint32_t value[2]) {
    (void)ctx;(void)address;(void)value;return 0;
}
int main(void) {
    struct port_exe_systime clock={0};
    port_exe_systime_store(&clock,UINT64_C(0x01020303ffffffff));
    struct test_writer retry={.clock=&clock};
    assert(port_exe_gettim(&clock,0x42u,write_snapshot,&retry)==PORT_SS_NORMAL);
    assert(retry.calls==2u);
    assert(retry.recorded[0]==0x7fffffffu && retry.recorded[1]==0x01020304u);
    assert(port_exe_gettim(&clock,0x42u,write_denied,0)==PORT_SS_ACCVIO);
    assert(port_exe_gettim(0,0x42u,write_snapshot,&retry)==PORT_SS_ACCVIO);
    assert(port_exe_gettim(&clock,0x42u,0,0)==PORT_SS_ACCVIO);

    struct port_user_map map={pd,table,0};
    struct port_exe_user_time u={&map,page,0};
    pd[0]=0x5000u|7u;
    pt[1]=0x8000u|7u;pt[2]=0x9000u|7u;
    memset(first,0xa5,sizeof(first));memset(second,0xa5,sizeof(second));
    /* The quadword crosses a real IA-32 page boundary. */
    assert(port_exe_gettim_user(&clock,0x1ffcu,&u)==PORT_SS_NORMAL);
    assert(first[4092]==0xffu && first[4093]==0xffu &&
           first[4094]==0xffu && first[4095]==0x7fu);
    assert(second[0]==4u && second[1]==3u && second[2]==2u && second[3]==1u);
    pt[2]=0x9000u|5u; /* Caller cannot write the second page. */
    memset(first,0xa5,sizeof(first));
    assert(port_exe_gettim_user(&clock,0x1ffcu,&u)==PORT_SS_ACCVIO);
    assert(first[4092]==0xa5u && first[4095]==0xa5u);
    pt[2]=0x9000u|7u;
    pd[0]=0x5000u|5u;
    assert(port_exe_gettim_user(&clock,0x1ffcu,&u)==PORT_SS_ACCVIO);
    pd[0]=0x5000u|7u;
    assert(port_exe_gettim_user(&clock,0xfffffffcu,&u)==PORT_SS_ACCVIO);
    assert(port_exe_gettim_user(&clock,0x1ffcu,0)==PORT_SS_ACCVIO);
    volatile uint8_t filter=0x5au;
    assert(port_exe_setssf(&filter,3u,0x12u)==PORT_SS_NOPRIV);
    assert(filter==0x5au);
    for(uint32_t mode=0;mode<3u;mode++) {
        assert(port_exe_setssf(&filter,mode,0x1357u+mode)==PORT_SS_NORMAL);
        assert(filter==(uint8_t)(0x57u+mode));
    }
    assert(port_exe_setssf(&filter,7u,0u)==PORT_SS_NOPRIV);
    assert(port_exe_setssf(0,0u,1u)==PORT_SS_NOPRIV);
    puts("SYSGETTIM and SYSSETSSF system-service tests PASS");
    return 0;
}

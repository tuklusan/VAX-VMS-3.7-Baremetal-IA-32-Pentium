#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "vms_exe_setexv.h"
static uint32_t pd[1024],pt[1024];
static uint8_t pages[2][4096];
static const uint32_t *resolve_table(void *ctx,uint32_t physical){
    (void)ctx;return physical==0x5000u?pt:0;
}
static uint8_t *resolve_page(void *ctx,uint32_t physical){
    (void)ctx;
    return physical==0x8000u?pages[0]:physical==0x9000u?pages[1]:0;
}
static int deny(void *ctx,uint32_t address,const uint32_t *old){
    (void)ctx;(void)address;(void)old;return 0;
}
int main(void) {
    struct port_exe_exception_vectors v={0};
    struct port_user_map m={pd,resolve_table,0};
    struct port_exe_user_vector u={&m,resolve_page,0};
    pd[0]=0x5000u|7u;pt[1]=0x8000u|7u;pt[2]=0x9000u|7u;
    /* Effective mode never exceeds the caller's privilege. */
    for(unsigned request=0;request<4u;request++)
      for(unsigned caller=0;caller<4u;caller++) {
        unsigned expected=request>caller?request:caller;
        memset(&v,0,sizeof(v));
        assert(port_exe_setexv_user(&v,0u,0x110000u+request,
                                   request,caller,0u,&u)==PORT_SS_NORMAL);
        assert(v.normal[expected][0]==0x110000u+request);
      }
    memset(&v,0,sizeof(v));
    assert(port_exe_setexv_user(&v,0u,0x12345678u,0u,3u,0u,&u)==PORT_SS_NORMAL);
    assert(port_exe_setexv_user(&v,1u,0xabcdef01u,0u,3u,0u,&u)==PORT_SS_NORMAL);
    assert(v.normal[3][0]==0x12345678u && v.normal[3][1]==0xabcdef01u);
    assert(port_exe_setexv_user(&v,2u,0x87654321u,0u,3u,0u,&u)==PORT_SS_NORMAL);
    assert(v.last[3]==0x87654321u);
    /* Bit one has priority over the bit-zero secondary indicator. */
    assert(port_exe_setexv_user(&v,3u,0u,0u,3u,0u,&u)==PORT_SS_NORMAL);
    assert(v.last[3]==0u && v.normal[3][1]==0xabcdef01u);
    memset(pages,0x55,sizeof(pages));
    assert(port_exe_setexv_user(&v,3u,0x20002u,0u,3u,0x1ffeu,&u)==PORT_SS_NORMAL);
    assert(v.last[3]==0x20002u);
    assert(pages[0][4094]==0 && pages[0][4095]==0 &&
           pages[1][0]==0 && pages[1][1]==0);
    /* An unwritable cross-page result must not set the new handler. */
    pt[2]=0x9000u|5u;
    memset(pages,0x55,sizeof(pages));
    assert(port_exe_setexv_user(&v,3u,0x30003u,0u,3u,0x1ffeu,&u)==PORT_SS_ACCVIO);
    assert(v.last[3]==0x20002u && pages[0][4094]==0x55u);
    pt[2]=0x9000u|7u;
    assert(port_exe_setexv_user(&v,2u,0x30003u,0u,3u,0x1ffeu,0)==PORT_SS_ACCVIO);
    assert(v.last[3]==0x20002u);
    assert(port_exe_setexv(&v,2u,0x30003u,0u,3u,0x1ffeu,deny,0)==PORT_SS_ACCVIO);
    assert(v.last[3]==0x20002u);
    assert(port_exe_setexv(0,0u,4u,0u,0u,0u,0,0)==PORT_SS_ACCVIO);
    /* Previous vector is returned and then replaced on valid access. */
    assert(port_exe_setexv_user(&v,2u,0x40004u,0u,3u,0x1000u,&u)==PORT_SS_NORMAL);
    uint32_t old=0;memcpy(&old,pages[0],4u);
    assert(old==0x20002u && v.last[3]==0x40004u);
    puts("SYSSETEXV access modes, vector selection and saved old handler PASS");
    return 0;
}

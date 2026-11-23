#include "vms_iopost_swi.h"
#include <assert.h>
#include <stdio.h>
static uint32_t callbacks;
static struct port_iopost_swi *nested_state;
static uint32_t good(void){++callbacks;return PORT_IOPOST_SW_OK;}
static uint32_t bad(void){++callbacks;return PORT_IOPOST_SW_BAD;}
static uint32_t recursive(void){
    ++callbacks;
    assert(port_iopost_swi_dispatch(nested_state,PORT_IOPOST_SW_VECTOR)==PORT_IOPOST_SW_BUSY);
    return PORT_IOPOST_SW_OK;
}
int main(void){
    struct port_iopost_swi s={0};
    assert(port_iopost_swi_dispatch(&s,PORT_IOPOST_SW_VECTOR)==PORT_IOPOST_SW_UNBOUND);
    assert(port_iopost_swi_bind(&s,good,0x320080u,0x320000u,0x380000u)==PORT_IOPOST_SW_OK);
    assert(port_iopost_swi_bind(&s,good,0x320080u,0x320000u,0x380000u)==PORT_IOPOST_SW_BAD);
    assert(port_iopost_swi_dispatch(&s,0x80u)==PORT_IOPOST_SW_BAD);
    assert(callbacks==0u&&s.calls==0u);
    assert(port_iopost_swi_dispatch(&s,PORT_IOPOST_SW_VECTOR)==PORT_IOPOST_SW_OK);
    assert(callbacks==1u);
    assert(port_iopost_swi_dispatch(&s,PORT_IOPOST_SW_VECTOR)==PORT_IOPOST_SW_UNBOUND);
    assert(port_iopost_swi_check(&s,1u,PORT_IOPOST_SW_OK)==PORT_IOPOST_SW_OK);
    assert(port_iopost_swi_check(&s,2u,PORT_IOPOST_SW_OK)==PORT_IOPOST_SW_CORRUPT);
    port_iopost_swi_init(&s);
    assert(port_iopost_swi_bind(&s,good,0x31ffffu,0x320000u,0x380000u)==PORT_IOPOST_SW_BAD);
    assert(port_iopost_swi_bind(&s,good,0x380000u,0x320000u,0x380000u)==PORT_IOPOST_SW_BAD);
    assert(port_iopost_swi_bind(&s,good,0x320100u,0x330000u,0x320000u)==PORT_IOPOST_SW_BAD);
    assert(port_iopost_swi_bind(&s,bad,0x320080u,0x320000u,0x380000u)==PORT_IOPOST_SW_OK);
    assert(port_iopost_swi_dispatch(&s,PORT_IOPOST_SW_VECTOR)==PORT_IOPOST_SW_CORRUPT);
    assert(port_iopost_swi_check(&s,1u,PORT_IOPOST_SW_BAD)==PORT_IOPOST_SW_OK);
    port_iopost_swi_init(&s);nested_state=&s;
    assert(port_iopost_swi_bind(&s,recursive,0x320100u,0x320000u,0x380000u)==PORT_IOPOST_SW_OK);
    assert(port_iopost_swi_dispatch(&s,PORT_IOPOST_SW_VECTOR)==PORT_IOPOST_SW_OK);
    assert(port_iopost_swi_check(&s,1u,PORT_IOPOST_SW_OK)==PORT_IOPOST_SW_OK);
    puts("IA-32 supervisor IDT 0x90 native IOCIOPOST single-use queue dispatch PASS");
    return 0;
}

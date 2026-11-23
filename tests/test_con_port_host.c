#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_con_port.h"
#include "vms_v37_dyn_tags.h"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#c);return 1;}}while(0)
static uint32_t calls;
static uint32_t succeeded(void *ctx){++*(uint32_t *)ctx;return PORT_V37_CON_OK;}
static uint32_t refused(void *ctx){++*(uint32_t *)ctx;return PORT_V37_CON_BAD;}
int main(void){
    uint32_t buffer[320/4]={0};uint8_t *u=(uint8_t *)buffer;
    struct port_v37_con_backend ok={succeeded,&calls},no={refused,&calls};
    u[8]=320u&255u;u[9]=320u>>8;u[10]=PORT_V37_DYN_UCB;
    CHECK(port_v37_con_valid(u,sizeof buffer)==PORT_V37_CON_OK);
    CHECK(port_v37_con_xoff(u,255u,&ok)==PORT_V37_CON_BAD);
    CHECK(port_v37_con_get16(u,PORT_V37_CON_HOLD)==0u && calls==0u);
    CHECK(port_v37_con_xoff(u,sizeof buffer,0)==PORT_V37_CON_DEFERRED);
    CHECK(port_v37_con_get16(u,PORT_V37_CON_HOLD)==PORT_V37_CON_XOFF);
    CHECK(!(port_v37_con_get16(u,PORT_V37_CON_STS)&PORT_V37_CON_INT));
    CHECK(port_v37_con_xon(u,sizeof buffer,&no)==PORT_V37_CON_DEFERRED);
    CHECK(calls==1u && port_v37_con_get16(u,PORT_V37_CON_HOLD)==(PORT_V37_CON_XOFF|PORT_V37_CON_XON));
    CHECK(port_v37_con_xon(u,sizeof buffer,&ok)==PORT_V37_CON_OK);
    CHECK(calls==2u && (port_v37_con_get16(u,PORT_V37_CON_STS)&PORT_V37_CON_INT));
    CHECK(port_v37_con_xoff(u,sizeof buffer,&ok)==PORT_V37_CON_OK && calls==2u);
    CHECK(port_v37_con_stop(u,sizeof buffer)==PORT_V37_CON_OK);
    CHECK(port_v37_con_get32(u,PORT_V37_CON_STATE)&PORT_V37_CON_CTRLS);
    CHECK(port_v37_con_get16(u,PORT_V37_CON_HOLD)&PORT_V37_CON_STOP);
    CHECK(port_v37_con_stop2(u,sizeof buffer,123u)==PORT_V37_CON_OK);
    CHECK(port_v37_con_get16(u,PORT_V37_CON_HOLD)&PORT_V37_CON_STOP2);
    CHECK(port_v37_con_get32(u,PORT_V37_CON_DUETIM)==125u);
    CHECK(port_v37_con_get16(u,PORT_V37_CON_STS)&PORT_V37_CON_TIM);
    port_v37_con_set16(u,PORT_V37_CON_HOLD,port_v37_con_get16(u,PORT_V37_CON_HOLD)|PORT_V37_CON_BURST);
    port_v37_con_set16(u,PORT_V37_CON_OUTLEN,17u);
    CHECK(port_v37_con_resume(u,sizeof buffer,200u,&ok)==PORT_V37_CON_OK);
    CHECK(calls==2u); /* INT active, do not call TERMINALIO again */
    CHECK(port_v37_con_get32(u,PORT_V37_CON_DUETIM)==206u);
    CHECK(!(port_v37_con_get32(u,PORT_V37_CON_STATE)&PORT_V37_CON_CTRLS));
    CHECK(!(port_v37_con_get16(u,PORT_V37_CON_HOLD)&(PORT_V37_CON_STOP|PORT_V37_CON_STOP2)));
    CHECK(port_v37_con_abort(u,sizeof buffer,UINT32_MAX)==PORT_V37_CON_OK);
    CHECK(!(port_v37_con_get16(u,PORT_V37_CON_HOLD)&PORT_V37_CON_BURST));
    CHECK(port_v37_con_get32(u,PORT_V37_CON_DUETIM)==1u);
    port_v37_con_set16(u,PORT_V37_CON_STS,0u);
    CHECK(port_v37_con_resume(u,sizeof buffer,UINT32_MAX,0)==PORT_V37_CON_DEFERRED);
    CHECK(port_v37_con_get32(u,PORT_V37_CON_DUETIM)==1u);
    CHECK(port_v37_con_resume(u,sizeof buffer,3u,&ok)==PORT_V37_CON_OK);
    CHECK(calls==3u && (port_v37_con_get16(u,PORT_V37_CON_STS)&PORT_V37_CON_INT));
    /* Every invalid request leaves data untouched, including source type. */
    u[10]=PORT_V37_DYN_DDB;
    uint8_t old[320];memcpy(old,u,sizeof old);
    CHECK(port_v37_con_stop2(u,sizeof buffer,20u)==PORT_V37_CON_BAD);
    CHECK(port_v37_con_abort(u,sizeof buffer,20u)==PORT_V37_CON_BAD);
    CHECK(port_v37_con_resume(u,sizeof buffer,20u,&ok)==PORT_V37_CON_BAD);
    CHECK(!memcmp(old,u,sizeof old));
    puts("CONINTDSP.MAR source terminal XON/XOFF/STOP/STOP2/ABORT/RESUME transitions PASS");
    return 0;
}

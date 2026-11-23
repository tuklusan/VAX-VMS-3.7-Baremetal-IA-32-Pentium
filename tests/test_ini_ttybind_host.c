#include "vms_ini_ttydrv.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
static void w16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static uint32_t r32(const uint8_t *p){
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|
           (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
int main(void){
    uint32_t ddb_words[24],ucb_words[96],op_dpt_words[24];
    uint8_t *ddb=(uint8_t *)ddb_words,*ucb=(uint8_t *)ucb_words,*op=(uint8_t *)op_dpt_words;
    memset(ddb,0xa5,sizeof ddb_words);
    memset(ucb,0xa5,sizeof ucb_words);
    memset(op,0xa5,sizeof op_dpt_words);
    w16(ucb+8u,312u);w16(op+8u,96u);op[10u]=0x37u;w16(op+30u,72u);
    const struct port_ini_tty_class c={
       .dpt_address=0x360000u,.vector_address=0x360060u,
       .ddt_address=0x360200u,.getnext_address=0x360180u,
       .putnext_address=0x3601a0u,.relocated_entries=6u,
       .required_console_ucb_bytes=244u};
    struct port_ini_tty_console_binding out={0};
    CHECK(port_ini_tty_bind_console(ddb,sizeof ddb_words,ucb,sizeof ucb_words,
           op,sizeof op_dpt_words,0x320000u,0x37u,&c,&out)==PORT_INI_TTY_OK);
    CHECK(out.class_ddt_address==c.ddt_address);
    CHECK(out.port_vector_address==0x320048u);
    CHECK(r32(ddb+12u)==c.ddt_address&&r32(ucb+120u)==c.ddt_address);
    CHECK(r32(ucb+200u)==c.getnext_address&&r32(ucb+204u)==c.putnext_address);
    CHECK(r32(ucb+208u)==c.vector_address&&r32(ucb+212u)==0x320048u);
    CHECK(ddb[11u]==0xa5u&&ddb[16u]==0xa5u&&ucb[199u]==0xa5u);
    uint8_t original_ddb[sizeof ddb_words],original_ucb[sizeof ucb_words];
    memcpy(original_ddb,ddb,sizeof original_ddb);
    memcpy(original_ucb,ucb,sizeof original_ucb);
    w16(ucb+8u,200u);
    CHECK(port_ini_tty_bind_console(ddb,sizeof ddb_words,ucb,sizeof ucb_words,
           op,sizeof op_dpt_words,0x320000u,0x37u,&c,&out)==PORT_INI_TTY_TOO_SMALL_UCB);
    CHECK(!memcmp(ddb,original_ddb,sizeof original_ddb));
    w16(ucb+8u,312u);
    CHECK(!memcmp(ucb,original_ucb,sizeof original_ucb));
    CHECK(port_ini_tty_bind_console(ddb,sizeof ddb_words,ucb,sizeof ucb_words,
           op,sizeof op_dpt_words,0x320000u,0x38u,&c,&out)==PORT_INI_TTY_BAD_DPT);
    CHECK(!memcmp(ddb,original_ddb,sizeof original_ddb));
    w16(op+30u,0u);
    CHECK(port_ini_tty_bind_console(ddb,sizeof ddb_words,ucb,sizeof ucb_words,
           op,sizeof op_dpt_words,0x320000u,0x37u,&c,&out)==PORT_INI_TTY_BAD_VECTOR);
    CHECK(!memcmp(ucb,original_ucb,sizeof original_ucb));
    puts("INIT.MAR original OPA0 DDB/UCB class/port vector binding PASS");
    return 0;
}

#include "vms_exe_consolio.h"
#include <stddef.h>
const char port_exe_hextab[16]="0123456789ABCDEF";
/* A console XOFF holds output until XON. The parity bit is ignored. */
static int flow_check(const struct port_exe_console *console,uint32_t device){
    if(device || !console->receive_ready || !console->receive)return 1;
    if(!console->receive_ready(console->context))return 1;
    uint8_t c=0;
    if(!console->receive(console->context,&c))return 0;
    if((c&127u)!=19u)return 1;
    for(;;){
        if(!console->receive_ready(console->context))continue;
        if(!console->receive(console->context,&c))return 0;
        if((c&127u)==17u)return 1;
    }
}
static int one(const struct port_exe_console *console,uint32_t device,uint8_t byte){
    if(!console->transmit || !console->transmit_ready)return 0;
    if(!flow_check(console,device))return 0;
    while(!console->transmit_ready(console->context,device)){}
    return console->transmit(console->context,device,byte);
}
int port_exe_outchar(const struct port_exe_console *console,uint32_t device,uint8_t byte){
    if(!console)return 0;
    if(!one(console,device,byte))return 0;
    /* Console output adds two NUL fill characters after carriage return. */
    if(device==0 && byte==13u){
        if(!one(console,device,0u))return 0;
        if(!one(console,device,0u))return 0;
    }
    return 1;
}
int port_exe_outblank(const struct port_exe_console *c,uint32_t device){return port_exe_outchar(c,device,' ');}
int port_exe_outcrlf(const struct port_exe_console *c,uint32_t device){
    return port_exe_outchar(c,device,13u)&&port_exe_outchar(c,device,10u);
}
int port_exe_outhex(const struct port_exe_console *c,uint32_t device,uint32_t value){
    for(unsigned shift=28;;shift-=4){
        if(!port_exe_outchar(c,device,(uint8_t)port_exe_hextab[(value>>shift)&15u]))return 0;
        if(!shift)break;
    }
    return 1;
}
int port_exe_outcstring(const struct port_exe_console *c,uint32_t device,const uint8_t *p){
    if(!p)return 0;
    const unsigned count=*p++;
    for(unsigned i=0;i<count && p[i];i++)if(!port_exe_outchar(c,device,p[i]))return 0;
    return 1;
}
int port_exe_outzstring(const struct port_exe_console *c,uint32_t device,const char *p){
    if(!p)return 0;
    for(unsigned i=0;i<255u && p[i];i++)if(!port_exe_outchar(c,device,(uint8_t)p[i]))return 0;
    return 1;
}

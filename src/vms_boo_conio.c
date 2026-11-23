#include "vms_boo_conio.h"
#include <stddef.h>
static int output(const struct port_boo_console_ops *ops,uint8_t c){
    if(!ops || !ops->write || !ops->available || !ops->read)return 0;
    while(ops->available(ops->context)){
        uint8_t pending=0;
        if(!ops->read(ops->context,&pending))return 0;
        if((pending&127u)==19u){
            for(;;){
                if(!ops->read(ops->context,&pending))return 0;
                if((pending&127u)==17u)break;
            }
        }
    }
    return ops->write(ops->context,c);
}
static int out_text(const struct port_boo_console_ops *ops,const char *p){
    if(!p)return 0;
    for(;*p;p++)if(!output(ops,(uint8_t)*p))return 0;
    return 1;
}
uint32_t port_boo_conio_prompt(const char *prompt,uint32_t maximum,
                               uint8_t *buf,const struct port_boo_console_ops *ops){
    if(!prompt || !ops || !ops->read || !ops->write || !ops->available ||
       maximum>255u || (maximum && !buf))return PORT_SS_BADPARAM;
    for(;;){
        if(!out_text(ops,prompt))return PORT_SS_ACCVIO;
        if(!maximum)return PORT_SS_NORMAL;
        unsigned used=0;
        buf[0]=0;
        uint8_t rubbing=0;
        while(used+1u<maximum){
            uint8_t c=0;
            if(!ops->read(ops->context,&c))return PORT_SS_ACCVIO;
            c&=127u;
            if(c==127u){
                if(used){
                    if(!rubbing && !output(ops,'\\'))return PORT_SS_ACCVIO;
                    rubbing=1;
                    --used;
                    if(!output(ops,buf[1u+used]))return PORT_SS_ACCVIO;
                }
                continue;
            }
            if(rubbing){if(!output(ops,'\\'))return PORT_SS_ACCVIO;rubbing=0;}
            if(c==21u)break; /* Ctrl-U restarts both prompt and buffer. */
            if(c&64u)c&=(uint8_t)~32u;
            if(c==13u){
                if(!output(ops,13u)||!output(ops,10u))return PORT_SS_ACCVIO;
                buf[0]=(uint8_t)used;
                return PORT_SS_NORMAL;
            }
            if(!output(ops,c))return PORT_SS_ACCVIO;
            buf[1u+used++]=c;
        }
        if(used+1u>=maximum){
            if(!output(ops,13u)||!output(ops,10u))return PORT_SS_ACCVIO;
            buf[0]=(uint8_t)used;
            return PORT_SS_NORMAL;
        }
    }
}

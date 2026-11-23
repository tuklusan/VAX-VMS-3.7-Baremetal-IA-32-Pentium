#include "vms_boo_readprompt.h"
#include <stddef.h>
uint32_t port_boo_readprompt(struct port_boo_readprompt *io,
                              const char *prompt,uint16_t size,uint8_t *counted){
    if(!io || !prompt || (size && !counted) || !io->assign ||
       !io->qiow_readprompt || !io->qiow_write)return PORT_SS_BADPARAM;
    uint32_t status=PORT_SS_NORMAL;
    if(!io->channel){
        status=io->assign(io->context,"_OPA0",&io->channel);
        if(!(status&1u))return status;
    }
    uint32_t length=0;
    while(length<65535u && prompt[length])length++;
    if(size){
        uint16_t count=0;
        status=io->qiow_readprompt(io->context,io->channel,prompt,(uint16_t)length,
                                  counted+1,size,&count);
        if(!(status&1u))return status;
        if(count>size || count>255u)return PORT_SS_BADPARAM;
        counted[0]=(uint8_t)count;
        return status;
    }
    return io->qiow_write(io->context,io->channel,(const uint8_t *)prompt,(uint16_t)length);
}
uint32_t port_boo_output_line(struct port_boo_readprompt *io){
    if(!io || io->output_length>252u)return PORT_SS_BADPARAM;
    io->output[io->output_length]=13;
    io->output[io->output_length+1]=10;
    io->output[io->output_length+2]=0;
    io->output[io->output_length+3]=0;
    return port_boo_readprompt(io,(const char *)io->output,0,0);
}

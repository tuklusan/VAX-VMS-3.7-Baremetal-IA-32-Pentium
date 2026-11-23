#include "vms_boo_sysgenmn.h"
#include <stddef.h>
uint32_t port_boo_sysgen_start(struct port_boo_sysgen *s,uint32_t cli_flags){
    if(!s || !s->lock_image || !s->cli_utility || !s->use_active ||
       !s->read_commands)return PORT_SS_BADPARAM;
    uint32_t code=s->lock_image(s->context);
    if(!(code&1u))return code;
    /* The original standalone front end passes a request with null return slots. */
    s->request_status=0;
    (void)s->cli_utility(s->context,&s->request_status);
    if(s->request_status!=s->foreign_verb){
        s->command_length=0;
        s->command_address=0;
    }
    /* CLI bit 2 is the original /VERIFY value. */
    uint8_t verify=(uint8_t)((cli_flags>>2)&1u);
    (void)s->use_active(s->context);
    code=s->read_commands(s->context,verify);
    return code==s->eof_status ? PORT_SS_NORMAL : code;
}
uint32_t port_boo_sysgen_puterror(struct port_boo_sysgen *s,uint32_t status,
                                  const char *facility){
    if(!s || !s->putmsg || !facility)return PORT_SS_BADPARAM;
    return s->putmsg(s->context,status,facility);
}

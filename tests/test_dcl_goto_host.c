#include <assert.h>
#include <string.h>
#include "vms_dcl_goto.h"
static unsigned calls;
static uint8_t found[6];
static int seek(void *ctx,const uint8_t rfa[6]){
    (void)ctx;calls++;
    memcpy(found,rfa,6);return 1;
}
static int fail(void *ctx,const uint8_t rfa[6]){
    (void)ctx;(void)rfa;return 0;
}
int main(void){
    struct port_dcl_goto_state p;
    struct port_dcl_goto_ops ops={seek};
    const uint8_t first[6]={1,2,3,4,5,6};
    const uint8_t second[6]={6,5,4,3,2,1};
    port_dcl_goto_init(&p);
    assert(port_dcl_goto(&p,"NO_FILE",0,&ops,0)==PORT_DCL_GOTO_NORMAL);
    assert(!p.pending&&p.no_status);
    assert(port_dcl_goto(&p," START ",1,&ops,0)==PORT_DCL_GOTO_NORMAL);
    assert(port_dcl_goto_skipping(&p));
    assert(port_dcl_goto_label_record(&p,"other",first)==0);
    assert(port_dcl_goto_skipping(&p));
    assert(port_dcl_goto_label_record(&p,"start",second)==PORT_DCL_GOTO_NORMAL);
    assert(!port_dcl_goto_skipping(&p));
    assert(port_dcl_goto(&p,"OTHER",1,&ops,0)==PORT_DCL_GOTO_NORMAL);
    assert(calls==1&&memcmp(found,first,6)==0&&!p.pending);
    ops.find_rfa=fail;
    assert(port_dcl_goto(&p,"start",1,&ops,0)==PORT_DCL_GOTO_FINDERR);
    assert(!p.pending);
    assert(port_dcl_goto(&p,"BAD LABEL",1,&ops,0)==PORT_DCL_GOTO_BADLABEL);
    assert(port_dcl_goto(&p,"",1,&ops,0)==PORT_DCL_GOTO_BADLABEL);
    assert(port_dcl_goto(&p,"unseen",1,&ops,0)==PORT_DCL_GOTO_NORMAL);
    assert(p.pending&&p.no_status);
    port_dcl_dealgoto(&p);
    assert(!p.pending&&!p.target[0]);
    assert(port_dcl_goto_label_record(&p,"other",second)==PORT_DCL_GOTO_NORMAL);
    return 0;
}

#include "vms_rem_protocol.h"

struct rem_selftest_ctx {
    uint16_t channel;
    unsigned sends,kills,verifies,starts,ast;
    unsigned length;
    uint8_t payload[20];
};
static int rem_test_send(void *ctx,uint16_t channel,const uint8_t *p,
                         unsigned len,int ast) {
    struct rem_selftest_ctx *c=ctx;
    if(len>sizeof c->payload||channel!=51)return 0;
    ++c->sends;c->length=len;c->channel=channel;c->ast=(unsigned)ast;
    for(unsigned i=0;i<len;i++)c->payload[i]=p[i];
    return 1;
}
static int rem_test_verify(void *ctx,uint16_t channel,uintptr_t *window) {
    struct rem_selftest_ctx *c=ctx;
    if(channel!=51)return 0;
    ++c->verifies;*window=(uintptr_t)0x12345678u;
    return 1;
}
static int rem_test_attention(void *ctx,unsigned mod) {
    struct rem_selftest_ctx *c=ctx;
    if(mod!=PORT_REM_ATTN_START_RECV)return 0;
    ++c->starts;return 1;
}
static void rem_test_kill(void *ctx,uint16_t channel) {
    struct rem_selftest_ctx *c=ctx;
    if(channel==51)++c->kills;
}
int port_rem_protocol_guest_selftest(void) {
    struct port_rem_terminal t;
    struct rem_selftest_ctx c={0};
    static const struct port_rem_protocol_ops ops={
        rem_test_send,rem_test_verify,rem_test_attention,rem_test_kill
    };
    uint8_t msg[20]={1,1,1,0,7,0,4,0,0x11,0x12,0x13,0x14,
                     0x15,0x16,0x17,0x18,0x21,0x22,0x23,0x24};
    port_rem_protocol_init(&t,51);
    if(port_rem_send_config(&t,&ops,&c)!=PORT_REM_OK||
       c.sends!=1||c.ast||c.length!=8||c.payload[6]!=4)return 0;
    if(port_rem_protocol_receive(&t,msg,sizeof msg,&ops,&c)!=PORT_REM_OK||
       !t.online||t.eco!=1||t.devdepend2[3]!=0x24||
       t.network_window!=0x12345678u||c.starts!=1||c.verifies!=1)return 0;
    if(port_rem_protocol_receive(&t,msg,sizeof msg,&ops,&c)!=PORT_REM_THROUGH)
        return 0;
    port_rem_protocol_init(&t,51);
    msg[1]=0;
    if(port_rem_protocol_receive(&t,msg,sizeof msg,&ops,&c)!=PORT_REM_WRONG_VERSION||
       c.length!=2||c.payload[0]!=2||c.payload[1]!=1||!c.ast||
       !t.awaiting_disconnect||c.kills)return 0;
    port_rem_send_complete(&t,&ops,&c);
    if(c.kills!=1||t.awaiting_disconnect||t.online)return 0;
    msg[1]=1;msg[6]=0;
    port_rem_protocol_init(&t,51);
    if(port_rem_protocol_receive(&t,msg,sizeof msg,&ops,&c)!=PORT_REM_NO_COMMON_PROTOCOL||
       c.payload[1]!=2)return 0;
    port_rem_send_complete(&t,&ops,&c);
    if(c.kills!=2)return 0;
    port_rem_protocol_init(&t,51);
    msg[0]=9;
    if(port_rem_protocol_receive(&t,msg,sizeof msg,&ops,&c)!=PORT_REM_PROTOCOL_ERROR||
       c.kills!=3)return 0;
    return 1;
}

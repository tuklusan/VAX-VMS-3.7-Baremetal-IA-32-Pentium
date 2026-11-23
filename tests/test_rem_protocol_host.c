#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_rem_protocol.h"

struct history {
    unsigned sends,kills,attentions,verifies,completion;
    int send_ok,channel_ok,attention_ok;
    uint16_t channel;
    unsigned length;
    uint8_t last[24];
};
static int send_packet(void *p,uint16_t channel,const uint8_t *data,
                       unsigned length,int ast){
    struct history *h=p;h->sends++;h->channel=channel;h->length=length;
    h->completion=(unsigned)ast;
    assert(length<=sizeof h->last);
    memcpy(h->last,data,length);
    return h->send_ok;
}
static int verify_channel(void *p,uint16_t channel,uintptr_t *window){
    struct history *h=p;h->verifies++;assert(channel==73);
    *window=(uintptr_t)0x12aa55ccu;return h->channel_ok;
}
static int attention(void *p,unsigned code){
    struct history *h=p;h->attentions++;
    assert(code==PORT_REM_ATTN_START_RECV);return h->attention_ok;
}
static void kill(void *p,uint16_t channel){
    struct history *h=p;assert(channel==73);h->kills++;
}
static const struct port_rem_protocol_ops ops={
    send_packet,verify_channel,attention,kill
};
static struct port_rem_terminal t;
static struct history h;
static const uint8_t config[20]={1,1,1,0,7,0,4,0,
    12,13,14,15,16,17,18,19,20,21,22,23};
static void restart(void){
    memset(&h,0,sizeof h);h.send_ok=h.channel_ok=h.attention_ok=1;
    port_rem_protocol_init(&t,73);
}
int main(void){
    uint8_t b[20];
    restart();
    assert(port_rem_send_config(&t,&ops,&h)==PORT_REM_OK);
    assert(h.length==8&&!h.completion&&h.channel==73);
    assert(!memcmp(h.last,config,8));
    assert(port_rem_protocol_receive(&t,config,20,&ops,&h)==PORT_REM_OK);
    assert(t.online&&t.version==1&&t.eco==1&&t.peer_eco==1);
    assert(t.characteristics[0]==12&&t.characteristics[7]==19);
    assert(t.devdepend2[0]==20&&t.devdepend2[3]==23);
    assert(t.network_window==(uintptr_t)0x12aa55ccu);
    assert(h.verifies==1&&h.attentions==1&&h.kills==0);
    assert(port_rem_protocol_receive(&t,config,20,&ops,&h)==PORT_REM_THROUGH);
    restart();memcpy(b,config,sizeof b);b[2]=0;
    assert(port_rem_protocol_receive(&t,b,16,&ops,&h)==PORT_REM_OK);
    assert(!t.eco&&!t.devdepend2[0]&&t.online);
    restart();memcpy(b,config,sizeof b);b[1]=2;b[2]=9;
    assert(port_rem_protocol_receive(&t,b,20,&ops,&h)==PORT_REM_OK);
    assert(t.version==1&&t.eco==1&&t.peer_version==2&&t.peer_eco==9);
    restart();memcpy(b,config,sizeof b);b[1]=0;
    assert(port_rem_protocol_receive(&t,b,20,&ops,&h)==PORT_REM_WRONG_VERSION);
    assert(h.length==2&&h.last[0]==2&&h.last[1]==1&&h.completion);
    assert(!h.kills&&!t.online&&t.awaiting_disconnect);
    port_rem_send_complete(&t,&ops,&h);
    assert(h.kills==1&&!t.awaiting_disconnect);
    port_rem_send_complete(&t,&ops,&h);assert(h.kills==1);
    restart();memcpy(b,config,sizeof b);b[6]=0;
    assert(port_rem_protocol_receive(&t,b,20,&ops,&h)==PORT_REM_NO_COMMON_PROTOCOL);
    assert(h.last[0]==2&&h.last[1]==2&&t.awaiting_disconnect);
    port_rem_send_complete(&t,&ops,&h);assert(h.kills==1);
    restart();memcpy(b,config,sizeof b);b[0]=9;
    assert(port_rem_protocol_receive(&t,b,20,&ops,&h)==PORT_REM_PROTOCOL_ERROR);
    assert(h.kills==1&&!h.sends);
    restart();assert(port_rem_protocol_receive(&t,config,19,&ops,&h)==PORT_REM_PROTOCOL_ERROR);
    assert(h.kills==1);
    restart();h.channel_ok=0;
    assert(port_rem_protocol_receive(&t,config,20,&ops,&h)==PORT_REM_CHANNEL_FAILURE);
    assert(!t.online&&h.kills==1&&!h.attentions);
    restart();h.attention_ok=0;
    assert(port_rem_protocol_receive(&t,config,20,&ops,&h)==PORT_REM_CHANNEL_FAILURE);
    assert(!t.online&&h.kills==1&&h.attentions==1);
    restart();h.send_ok=0;
    assert(port_rem_send_config(&t,&ops,&h)==PORT_REM_SEND_FAILURE);
    memcpy(b,config,sizeof b);b[1]=0;
    assert(port_rem_protocol_receive(&t,b,20,&ops,&h)==PORT_REM_SEND_FAILURE);
    assert(h.kills==1&&!t.awaiting_disconnect);
    puts("REM PROTOCOL host configuration, version, ECO, disconnect and channel checks PASS");
    return 0;
}

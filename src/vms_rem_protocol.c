#include "vms_rem_protocol.h"

static void rem_kill(struct port_rem_terminal *t,
                     const struct port_rem_protocol_ops *ops,void *ctx) {
    t->online=0;
    t->awaiting_disconnect=0;
    t->network_window=0;
    if(ops->kill)ops->kill(ctx,t->network_channel);
}

void port_rem_protocol_init(struct port_rem_terminal *t,uint16_t channel) {
    if(!t)return;
    *t=(struct port_rem_terminal){0};
    t->network_channel=channel;
}

/* REM$SEND_CONFIG: the initial eight-byte message is sent without an AST. */
int port_rem_send_config(struct port_rem_terminal *t,
                         const struct port_rem_protocol_ops *ops,void *ctx) {
    static const uint8_t configure[PORT_REM_CONFIG_BYTES]={
        PORT_REM_CONFIG_TYPE,PORT_REM_CURRENT_VERSION,PORT_REM_CURRENT_ECO,
        0,PORT_REM_VMS_SYSTEM,0,PORT_REM_VMS_SUPPORT,0
    };
    if(!t||!t->network_channel||!ops||!ops->send||t->awaiting_disconnect)
        return PORT_REM_EINVAL;
    return ops->send(ctx,t->network_channel,configure,sizeof configure,0)>0
        ?PORT_REM_OK:PORT_REM_SEND_FAILURE;
}

/* SEND_DISCON: a reason byte follows type 2. The send-completion AST must
 * remove the UCB; neither a network ACK nor a fabricated result does so. */
static int rem_disconnect(struct port_rem_terminal *t,uint8_t reason,
                          const struct port_rem_protocol_ops *ops,void *ctx,
                          int status) {
    const uint8_t message[PORT_REM_DISCONNECT_BYTES]={
        PORT_REM_DISCONNECT_TYPE,reason
    };
    t->online=0;
    t->disconnect_reason=reason;
    t->awaiting_disconnect=1;
    if(ops->send(ctx,t->network_channel,message,sizeof message,1)<=0) {
        rem_kill(t,ops,ctx);
        return PORT_REM_SEND_FAILURE;
    }
    return status;
}

/* REM$PROTOCOL: the CONFIGURE exchange ends with the RTT receive attention.
 * Later data messages belong to RTTDRIVER, not to this setup routine. */
int port_rem_protocol_receive(struct port_rem_terminal *t,
                              const uint8_t *message,unsigned length,
                              const struct port_rem_protocol_ops *ops,void *ctx) {
    if(!t||!message||!ops||!ops->kill||!ops->send||
       !ops->verify_channel||!ops->attention||!t->network_channel)
        return PORT_REM_EINVAL;
    if(t->online)return PORT_REM_THROUGH;
    if(t->awaiting_disconnect)return PORT_REM_EINVAL;
    if(length<PORT_REM_CONFIG_BYTES||message[0]!=PORT_REM_CONFIG_TYPE) {
        rem_kill(t,ops,ctx);
        return PORT_REM_PROTOCOL_ERROR;
    }
    t->peer_version=message[1];
    if(t->peer_version<PORT_REM_CURRENT_VERSION)
        return rem_disconnect(t,1,ops,ctx,PORT_REM_WRONG_VERSION);
    t->peer_eco=message[2];
    /* BITW the template's VMS support bit against the received support word. */
    if(!(message[6]&PORT_REM_VMS_SUPPORT))
        return rem_disconnect(t,2,ops,ctx,PORT_REM_NO_COMMON_PROTOCOL);
    t->version=PORT_REM_CURRENT_VERSION;
    t->eco=(t->peer_version>PORT_REM_CURRENT_VERSION ||
            t->peer_eco>PORT_REM_CURRENT_ECO)
           ?PORT_REM_CURRENT_ECO:t->peer_eco;
    if(length<PORT_REM_CONFIG_BYTES+PORT_REM_CHAR_BYTES+
              (t->eco?PORT_REM_EXT_CHAR_BYTES:0u)) {
        rem_kill(t,ops,ctx);
        return PORT_REM_PROTOCOL_ERROR;
    }
    for(unsigned i=0;i<PORT_REM_CHAR_BYTES;i++)
        t->characteristics[i]=message[PORT_REM_CONFIG_BYTES+i];
    if(t->eco)for(unsigned i=0;i<PORT_REM_EXT_CHAR_BYTES;i++)
        t->devdepend2[i]=message[PORT_REM_CONFIG_BYTES+PORT_REM_CHAR_BYTES+i];
    uintptr_t window=0;
    if(ops->verify_channel(ctx,t->network_channel,&window)<=0) {
        rem_kill(t,ops,ctx);
        return PORT_REM_CHANNEL_FAILURE;
    }
    t->network_window=window;
    t->online=1;
    if(ops->attention(ctx,PORT_REM_ATTN_START_RECV)<=0) {
        rem_kill(t,ops,ctx);
        return PORT_REM_CHANNEL_FAILURE;
    }
    return PORT_REM_OK;
}

/* DISCON_AST: called only when the asynchronous DISCONNECT write completes. */
void port_rem_send_complete(struct port_rem_terminal *t,
                            const struct port_rem_protocol_ops *ops,void *ctx) {
    if(t&&ops&&t->awaiting_disconnect)rem_kill(t,ops,ctx);
}

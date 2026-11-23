#ifndef VMS_REM_PROTOCOL_H
#define VMS_REM_PROTOCOL_H
#include <stdint.h>

/* REM/SRC/REMPROTCL.MAR: the REMACP and RTTDRIVER configure exchange. */
#define PORT_REM_CURRENT_VERSION 1u
#define PORT_REM_CURRENT_ECO 1u
#define PORT_REM_CONFIG_TYPE 1u
#define PORT_REM_DISCONNECT_TYPE 2u
#define PORT_REM_VMS_SYSTEM 7u
#define PORT_REM_VMS_SUPPORT 4u
#define PORT_REM_CONFIG_BYTES 8u
#define PORT_REM_CHAR_BYTES 8u
#define PORT_REM_EXT_CHAR_BYTES 4u
#define PORT_REM_DISCONNECT_BYTES 2u
#define PORT_REM_ATTN_START_RECV 1u

enum port_rem_result {
    PORT_REM_OK=1,
    PORT_REM_THROUGH=2,
    PORT_REM_DISCONNECT_QUEUED=3,
    PORT_REM_EINVAL=-1,
    PORT_REM_PROTOCOL_ERROR=-2,
    PORT_REM_WRONG_VERSION=-3,
    PORT_REM_NO_COMMON_PROTOCOL=-4,
    PORT_REM_SEND_FAILURE=-5,
    PORT_REM_CHANNEL_FAILURE=-6
};

struct port_rem_terminal {
    uint16_t network_channel;
    uint8_t online;
    uint8_t awaiting_disconnect;
    uint8_t peer_version;
    uint8_t peer_eco;
    uint8_t version;
    uint8_t eco;
    uint8_t disconnect_reason;
    uint8_t characteristics[PORT_REM_CHAR_BYTES];
    uint8_t devdepend2[PORT_REM_EXT_CHAR_BYTES];
    uintptr_t network_window;
};

struct port_rem_protocol_ops {
    /* Issue network I/O on the REMACP channel; a positive result means queued. */
    int (*send)(void *ctx,uint16_t channel,const uint8_t *data,
                unsigned length,int completion_ast);
    /* Native IOC$VERIFYCHAN returns the window associated with this channel. */
    int (*verify_channel)(void *ctx,uint16_t channel,uintptr_t *window);
    /* Deliver the RTT receive-start request to the remote terminal driver. */
    int (*attention)(void *ctx,unsigned modifier);
    /* REM$KILL_UCB tears down the channel and its terminal. */
    void (*kill)(void *ctx,uint16_t channel);
};

void port_rem_protocol_init(struct port_rem_terminal *t,uint16_t channel);
int port_rem_send_config(struct port_rem_terminal *t,
                         const struct port_rem_protocol_ops *ops,void *ctx);
int port_rem_protocol_receive(struct port_rem_terminal *t,
                              const uint8_t *message,unsigned length,
                              const struct port_rem_protocol_ops *ops,void *ctx);
void port_rem_send_complete(struct port_rem_terminal *t,
                            const struct port_rem_protocol_ops *ops,void *ctx);
#endif

#ifndef VMS37_NATIVE_CCB_H
#define VMS37_NATIVE_CCB_H
#include <stddef.h>
#include <stdint.h>

/* Source SYSDEF.MDL CCB is four longwords. CTL$GL_CCBBASE indexes backward
 * by the channel's byte offset (the channel token is a 16-byte multiple). */
enum {
    PORT_V37_CCB_BYTES=16u,
    PORT_V37_CCB_UCB=0u,
    PORT_V37_CCB_WIND=4u,
    PORT_V37_CCB_STS=8u,
    PORT_V37_CCB_AMOD=9u,
    PORT_V37_CCB_IOC=10u,
    PORT_V37_CCB_DIRP=12u,
    PORT_V37_CCB_PENDING=1u,
    PORT_V37_CCB_OK=1u,
    PORT_V37_CCB_BAD=2u,
    PORT_V37_CCB_IVCHAN=4u,
    PORT_V37_CCB_NOPRIV=6u
};

struct port_v37_ccb_view {
    uint8_t *ccb;
    uint32_t ucb,window;
    uint16_t outstanding;
    uint8_t status,access_limit,pending,reserved;
};

uint32_t port_v37_ccb_resolve(uint8_t *ccb_base,size_t preceding_bytes,
    uint32_t channel,uint16_t maximum_channel_offset,uint8_t caller_mode,
    struct port_v37_ccb_view *);
#endif

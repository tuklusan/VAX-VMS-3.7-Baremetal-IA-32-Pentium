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
    PORT_V37_CCB_NOPRIV=6u,
    PORT_V37_CCB_NOCHANNEL=8u
};
/* SYSDEFQZ.MDL UCB fixed fields used by the local EXE$ASSIGN path. */
enum { PORT_V37_UCB_PID=40u,PORT_V37_UCB_DEVCHAR=52u,
       PORT_V37_UCB_REFC=84u,PORT_V37_UCB_STATUS=88u,
       PORT_V37_UCB_FIXED_BYTES=132u,
       PORT_V37_UCB_TEMPLATE=1u<<13,
       PORT_V37_UCB_DEV_SPL=1u<<6,PORT_V37_UCB_DEV_FOD=1u<<14,
       PORT_V37_UCB_DEV_SHR=1u<<16,
       PORT_V37_ASSIGN_DEVALLOC=10u,PORT_V37_ASSIGN_NOPRIV=12u,
       PORT_V37_ASSIGN_NONLOCAL=14u,PORT_V37_ASSIGN_UNSUPPORTED=16u };

struct port_v37_ccb_view {
    uint8_t *ccb;
    uint32_t ucb,window;
    uint16_t outstanding;
    uint8_t status,access_limit,pending,reserved;
};

uint32_t port_v37_ccb_resolve(uint8_t *ccb_base,size_t preceding_bytes,
    uint32_t channel,uint16_t maximum_channel_offset,uint8_t caller_mode,
    struct port_v37_ccb_view *);
uint32_t port_v37_ccb_find_free(uint8_t *ccb_base,size_t preceding_bytes,
    uint16_t channel_count,uint16_t *high_water_channel,
    uint16_t *channel,uint8_t **ccb);

/* SYSASSIGN.MAR local-device branch. Caller supplies the already resolved
 * local UCB and the source access/ownership checks; the routine performs
 * IOC$FFCHAN selection, locked owner/share policy, and source CCB/UCB
 * reference updates. A nonzero mailbox is deliberately unsupported until
 * TEST_MAILBOX and mailbox association are wired. PORT_V37_ASSIGN_* returns
 * are internal helper codes; the public service must translate them to SS$_*. */
struct port_v37_assign_ops {
    void *context;
    uint32_t (*lock_io_database)(void *);
    void (*unlock_io_database)(void *);
    uint32_t (*owner_is_related)(void *,uint32_t,uint32_t);
    uint32_t (*check_allocate_access)(void *,uint32_t,const uint8_t *,size_t);
};
uint32_t port_v37_ccb_assign_local(uint8_t *ccb_base,size_t preceding_bytes,
    uint16_t channel_count,uint16_t *high_water_channel,uint8_t *ucb,
    size_t ucb_bytes,uint32_t ucb_va,uint32_t process_pid,uint32_t arb_va,
    uint8_t effective_max_acmode,uint32_t mailbox_ucb,
    const struct port_v37_assign_ops *ops,uint16_t *channel);
#endif

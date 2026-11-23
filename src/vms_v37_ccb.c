#include "vms_v37_ccb.h"

static uint16_t rd16(const uint8_t *p){
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));
}
static uint32_t rd32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}

/* SYS/SRC/SYSQIOREQ.MAR VCHAN: retain only channel bits 4..15, reject zero
 * and offsets past CTL$GW_CHINDX, then address CTL$GL_CCBBASE-channel.
 * The AMOD comparison is strict: equal or less privileged callers fail. */
uint32_t port_v37_ccb_resolve(uint8_t *base,size_t preceding_bytes,
    uint32_t channel,uint16_t maximum,uint8_t caller_mode,
    struct port_v37_ccb_view *view){
    if(!base||!view||caller_mode>3u||maximum==0u||
       ((uintptr_t)base&3u))return PORT_V37_CCB_BAD;
    const uint32_t offset=channel&UINT32_C(0x0000fff0);
    if((channel&UINT32_C(0xffff000f))||offset==0u||
       offset>maximum||offset>preceding_bytes||
       (offset%PORT_V37_CCB_BYTES)!=0u)
        return PORT_V37_CCB_IVCHAN;
    uint8_t *const ccb=base-offset;
    const uint32_t ucb=rd32(ccb+PORT_V37_CCB_UCB);
    const uint8_t limit=ccb[PORT_V37_CCB_AMOD];
    if(!ucb||limit==0u||limit>4u)return PORT_V37_CCB_IVCHAN;
    if(caller_mode>=limit)return PORT_V37_CCB_NOPRIV;
    const uint32_t window=rd32(ccb+PORT_V37_CCB_WIND);
    const struct port_v37_ccb_view out={
        .ccb=ccb,.ucb=ucb,.window=window,
        .outstanding=rd16(ccb+PORT_V37_CCB_IOC),
        .status=ccb[PORT_V37_CCB_STS],.access_limit=limit,
        .pending=(uint8_t)((window&PORT_V37_CCB_PENDING)!=0u)};
    *view=out;
    return PORT_V37_CCB_OK;
}

/* IOSUBPAGD.MAR IOC$FFCHAN: scan CCB$B_AMOD backwards from CTL$GL_CCBBASE,
 * return the first free byte-offset channel and raise CTL$GW_CHINDX. */
uint32_t port_v37_ccb_find_free(uint8_t *base,size_t preceding_bytes,
    uint16_t count,uint16_t *high_water,uint16_t *channel,uint8_t **out_ccb){
    if(!base||!high_water||!channel||!out_ccb||((uintptr_t)base&3u))
        return PORT_V37_CCB_BAD;
    if(!count)return PORT_V37_CCB_NOCHANNEL;
    for(uint32_t n=1u;n<=count;n++){
        const uint32_t offset=n*PORT_V37_CCB_BYTES;
        if(offset>UINT16_MAX||offset>preceding_bytes)
            return PORT_V37_CCB_BAD;
        uint8_t *const candidate=base-offset;
        if(candidate[PORT_V37_CCB_AMOD])continue;
        const uint16_t token=(uint16_t)offset;
        if(token>=*high_water)*high_water=token;
        *channel=token;
        *out_ccb=candidate;
        return PORT_V37_CCB_OK;
    }
    return PORT_V37_CCB_NOCHANNEL;
}

static uint16_t rd16u(const uint8_t *p){
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));
}
static void wr16u(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void wr32u(uint8_t *p,uint32_t v){
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}

/* SYSASSIGN.MAR FREECHAN/LOCAL through CCB$L_UCB and UCB$W_REFC update.
 * The name/descriptor search and TEST_MAILBOX are caller responsibilities;
 * network/template UCBs and mailbox association remain explicit exclusions. */
uint32_t port_v37_ccb_assign_local(uint8_t *base,size_t preceding_bytes,
    uint16_t channel_count,uint16_t *high_water,uint8_t *ucb,size_t ucb_bytes,
    uint32_t ucb_va,uint32_t pid,uint32_t arb_va,uint8_t effective_max_acmode,
    uint32_t mailbox_ucb,const struct port_v37_assign_ops *ops,
    uint16_t *channel){
    if(!base||!high_water||!ucb||!ucb_va||!pid||effective_max_acmode>3u||
       ucb_bytes<PORT_V37_UCB_FIXED_BYTES||!ops||!ops->lock_io_database||
       !ops->unlock_io_database||!ops->owner_is_related||!channel)
        return PORT_V37_CCB_BAD;
    if(mailbox_ucb)return PORT_V37_ASSIGN_UNSUPPORTED;
    const uint16_t size=rd16u(ucb+8u);
    if(size<PORT_V37_UCB_FIXED_BYTES||size>ucb_bytes)return PORT_V37_CCB_BAD;
    if(rd16u(ucb+PORT_V37_UCB_STATUS)&PORT_V37_UCB_TEMPLATE)
        return PORT_V37_ASSIGN_NONLOCAL;

    uint16_t candidate_channel=0u;uint8_t *candidate_ccb=0;
    uint32_t status=port_v37_ccb_find_free(base,preceding_bytes,
        channel_count,high_water,&candidate_channel,&candidate_ccb);
    if(status!=PORT_V37_CCB_OK)return status;
    status=ops->lock_io_database(ops->context);
    if(!(status&1u))return status;

    uint32_t result=PORT_V37_CCB_OK;
    const uint32_t owner=rd32(ucb+PORT_V37_UCB_PID);
    const uint32_t chars=rd32(ucb+PORT_V37_UCB_DEVCHAR);
    if(candidate_ccb[PORT_V37_CCB_AMOD])result=PORT_V37_CCB_NOCHANNEL;
    else if(owner && !ops->owner_is_related(ops->context,owner,pid))
        result=PORT_V37_ASSIGN_DEVALLOC;
    else if(!owner && !(chars&(PORT_V37_UCB_DEV_SHR|
                               PORT_V37_UCB_DEV_SPL)) &&
            (!ops->check_allocate_access || !arb_va ||
             !(ops->check_allocate_access(ops->context,arb_va,ucb,
                                            ucb_bytes)&1u)))
        result=PORT_V37_ASSIGN_NOPRIV;
    if(result==PORT_V37_CCB_OK){
        if(!owner && !(chars&(PORT_V37_UCB_DEV_SHR|PORT_V37_UCB_DEV_SPL)))
            wr32u(ucb+PORT_V37_UCB_PID,pid);
        wr32u(candidate_ccb+PORT_V37_CCB_UCB,ucb_va);
        wr16u(ucb+PORT_V37_UCB_REFC,
            (uint16_t)(rd16u(ucb+PORT_V37_UCB_REFC)+1u));
        candidate_ccb[PORT_V37_CCB_AMOD]=
            (uint8_t)(effective_max_acmode+1u);
        candidate_ccb[PORT_V37_CCB_STS]=0u;
        *channel=candidate_channel;
    }
    ops->unlock_io_database(ops->context);
    return result;
}

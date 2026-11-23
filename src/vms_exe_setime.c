#include "vms_exe_setime.h"
#include "vms_logintime.h"
#include <stddef.h>
static uint64_t read_clock(const struct port_exe_systime *c) {
    uint32_t seq=0,lo=0,hi=0;
    do {
        seq=__atomic_load_n(&c->sequence,__ATOMIC_ACQUIRE);
        if(seq&1u)continue;
        lo=__atomic_load_n(&c->low,__ATOMIC_RELAXED);
        hi=__atomic_load_n(&c->high,__ATOMIC_RELAXED);
    }while(seq!=__atomic_load_n(&c->sequence,__ATOMIC_ACQUIRE));
    return ((uint64_t)hi<<32)|lo;
}
/* IA-32 freestanding executive: 64-bit quotient without compiler runtime. */
static uint64_t tick_quotient(uint64_t value){
    uint64_t quotient=0,rest=0;
    for(unsigned i=0;i<64;i++){
        rest=(rest<<1)|(value>>63);
        value<<=1;
        quotient<<=1;
        if(rest>=PORT_EXE_TICK_100NS){
            rest-=PORT_EXE_TICK_100NS;
            quotient|=1u;
        }
    }
    return quotient;
}
static int year_for(uint64_t t,unsigned *year,uint64_t *base,uint64_t *length){
    unsigned lo=1858,hi=9999;
    while(lo<hi){
        unsigned mid=lo+(hi-lo+1u)/2u;
        uint64_t v=0;
        if(!port_time_calendar_to_ticks(mid,1,1,0,0,0,&v))return 0;
        if(t>=v)lo=mid;else hi=mid-1u;
    }
    uint64_t first=0,last=0;
    if(!port_time_calendar_to_ticks(lo,1,1,0,0,0,&first))return 0;
    if(lo==9999)return 0;
    if(!port_time_calendar_to_ticks(lo+1u,1,1,0,0,0,&last))return 0;
    *year=lo;*base=first;*length=last-first;
    return t>=first && t<last;
}
uint32_t port_exe_setime(struct port_exe_time_state *s,uint32_t address,
                         uint8_t oper,uint8_t logio,
                         const struct port_exe_setime_ops *ops) {
    if(!oper || !logio)return PORT_SS_NOPRIV;
    if(!s || !s->clock || !s->timers || !ops || !ops->memory ||
       !ops->read_todr || !ops->write_todr || !ops->save_sysparam ||
       !ops->check_bootcb || !ops->lock_clock || !ops->unlock_clock)
        return PORT_SS_BADPARAM;
    uint64_t incoming=0,final;
    uint32_t current_todr=0;
    if(address && !port_exe_read(ops->memory,address,&incoming,8u))
        return PORT_SS_ACCVIO;
    if((incoming>>63)!=0u)return PORT_SS_IVTIME;
    unsigned year;
    uint64_t first=0,year_len=0;
    uint32_t status;
    if(incoming){
        final=incoming;
        if(!year_for(final,&year,&first,&year_len))return PORT_SS_IVTIME;
        uint64_t offset=tick_quotient(final-first);
        if(offset>UINT32_MAX-PORT_EXE_TODR_BIAS)return PORT_SS_IVTIME;
        current_todr=PORT_EXE_TODR_BIAS+(uint32_t)offset;
    }else {
        if(!s->base_time)return PORT_SS_IVTIME;
        if(!year_for(s->base_time,&year,&first,&year_len))return PORT_SS_IVTIME;
        status=ops->read_todr(ops->context,&current_todr);
        if(!(status&1u))return status;
        if(current_todr<PORT_EXE_TODR_BIAS)return PORT_SS_IVTIME;
        uint32_t old_todr=s->todr;
        if(old_todr<PORT_EXE_TODR_BIAS)return PORT_SS_IVTIME;
        uint32_t years_ticks=(uint32_t)tick_quotient(year_len);
        if(current_todr>=PORT_EXE_TODR_BIAS+years_ticks)
            current_todr-=years_ticks;
        if(current_todr<old_todr || current_todr-old_todr>years_ticks)
            return PORT_SS_IVTIME;
        uint64_t delta=(uint64_t)(current_todr-old_todr)*PORT_EXE_TICK_100NS;
        if(UINT64_MAX-s->base_time<delta)return PORT_SS_IVTIME;
        final=s->base_time+delta;
    }
    ops->lock_clock(ops->context);
    status=ops->write_todr(ops->context,current_todr);
    if(!(status&1u)){ops->unlock_clock(ops->context);return status;}
    status=ops->read_todr(ops->context,&current_todr);
    if(!(status&1u)){ops->unlock_clock(ops->context);return status;}
    uint64_t previous=read_clock(s->clock);
    s->todr=current_todr;
    s->base_time=final;
    port_exe_systime_store(s->clock,final);
    /* Absolute timers stay fixed. Delta timers preserve time remaining. */
    for(unsigned i=0;i<PORT_EXE_TQE_CAPACITY;i++){
        struct port_exe_timer_entry *e=&s->timers->entries[i];
        if(!e->kind || e->absolute)continue;
        if(final>=previous){
            uint64_t delta=final-previous;
            e->when=(UINT64_MAX-e->when<delta)?UINT64_MAX:e->when+delta;
        }else {
            uint64_t delta=previous-final;
            e->when=(e->when<delta)?0u:e->when-delta;
        }
    }
    s->timers->now=final;
    ops->unlock_clock(ops->context);
    status=ops->check_bootcb(ops->context);
    if(!(status&1u))return status;
    return ops->save_sysparam(ops->context,current_todr,final);
}

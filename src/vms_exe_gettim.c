#include "vms_exe_gettim.h"

void port_exe_systime_store(struct port_exe_systime *clock,uint64_t time) {
    if(!clock)return;
    /* A 32-bit CPU cannot publish the complete quadword in one store. */
    __atomic_add_fetch(&clock->sequence,1u,__ATOMIC_RELEASE);
    __atomic_store_n(&clock->low,(uint32_t)time,__ATOMIC_RELAXED);
    __atomic_store_n(&clock->high,(uint32_t)(time>>32),__ATOMIC_RELAXED);
    __atomic_add_fetch(&clock->sequence,1u,__ATOMIC_RELEASE);
}
void port_exe_systime_tick(struct port_exe_systime *clock) {
    if(!clock)return;
    uint32_t low=clock->low;
    uint32_t high=clock->high;
    uint32_t next=low+UINT32_C(100000);
    if(next<low)++high;
    port_exe_systime_store(clock,((uint64_t)high<<32)|next);
}
uint32_t port_exe_gettim(const struct port_exe_systime *clock,
                         uint32_t address,port_exe_time_write write,
                         void *context) {
    if(!clock||!write)return PORT_SS_ACCVIO;
    for(;;) {
        uint32_t before=__atomic_load_n(&clock->sequence,__ATOMIC_ACQUIRE);
        if(before&1u)continue;
        uint32_t value[2]={
            __atomic_load_n(&clock->low,__ATOMIC_RELAXED),
            __atomic_load_n(&clock->high,__ATOMIC_RELAXED)
        };
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
        if(__atomic_load_n(&clock->sequence,__ATOMIC_ACQUIRE)!=before)
            continue;
        if(!write(context,address,value))return PORT_SS_ACCVIO;
        /* The original service retries if either half changes on copy. */
        if(__atomic_load_n(&clock->sequence,__ATOMIC_ACQUIRE)==before)
            return PORT_SS_NORMAL;
    }
}
static int write_user_time(void *opaque,uint32_t address,
                           const uint32_t value[2]) {
    const struct port_exe_user_time *u=(const struct port_exe_user_time *)opaque;
    return u && port_user_copy_to(u->map,address,value,8u,u->physical,
                                  u->context)==PORT_USER_OK;
}
uint32_t port_exe_gettim_user(const struct port_exe_systime *clock,
                              uint32_t address,
                              const struct port_exe_user_time *user) {
    if(!user)return PORT_SS_ACCVIO;
    return port_exe_gettim(clock,address,write_user_time,(void *)user);
}

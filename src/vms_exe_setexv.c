#include "vms_exe_setexv.h"
uint32_t port_exe_setexv(struct port_exe_exception_vectors *vectors,
                          uint32_t vector,uint32_t handler,
                          uint32_t requested_mode,uint32_t previous_mode,
                          uint32_t previous_address,
                          port_exe_word_write write,void *context) {
    if(!vectors)return PORT_SS_ACCVIO;
    uint32_t mode=requested_mode&3u;
    uint32_t caller=previous_mode&3u;
    if(mode<caller)mode=caller;
    uint32_t *slot=(vector&2u) ? &vectors->last[mode] :
                     &vectors->normal[mode][vector&1u];
    if(previous_address && (!write || !write(context,previous_address,slot)))
        return PORT_SS_ACCVIO;
    *slot=handler;
    return PORT_SS_NORMAL;
}
static int write_user_word(void *opaque,uint32_t address,
                           const uint32_t *value) {
    const struct port_exe_user_vector *u=(const struct port_exe_user_vector *)opaque;
    return u && port_user_copy_to(u->map,address,value,4u,
                                  u->physical,u->context)==PORT_USER_OK;
}
uint32_t port_exe_setexv_user(struct port_exe_exception_vectors *vectors,
                              uint32_t vector,uint32_t handler,
                              uint32_t requested_mode,uint32_t previous_mode,
                              uint32_t previous_address,
                              const struct port_exe_user_vector *user) {
    if(previous_address && !user)return PORT_SS_ACCVIO;
    return port_exe_setexv(vectors,vector,handler,requested_mode,
                            previous_mode,previous_address,write_user_word,
                            (void *)user);
}

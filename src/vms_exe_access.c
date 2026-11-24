#include "vms_exe_access.h"

static int user_read(void *context,uint32_t address,void *destination,
                     uint32_t bytes){
    struct port_exe_user_access *u=context;
    return u&&port_user_copy_from(u->map,destination,address,bytes,
                                  u->physical,u->physical_context)==PORT_USER_OK;
}
static int user_write(void *context,uint32_t address,const void *source,
                      uint32_t bytes){
    struct port_exe_user_access *u=context;
    return u&&port_user_copy_to(u->map,address,source,bytes,
                                u->physical,u->physical_context)==PORT_USER_OK;
}
static int user_can_read(void *context,uint32_t address,uint32_t bytes){
    struct port_exe_user_access *u=context;
    return u&&u->physical&&
        port_user_io_range_valid(u->map,address,bytes,0)==PORT_USER_OK;
}
static int user_can_write(void *context,uint32_t address,uint32_t bytes){
    struct port_exe_user_access *u=context;
    return u&&u->physical&&
        port_user_io_range_valid(u->map,address,bytes,1)==PORT_USER_OK;
}
static int user_lock_io(void *context,uint32_t address,uint32_t bytes,
                        int writing,struct port_user_page_locks *locks){
    struct port_exe_user_access *u=context;
    return u&&u->pin&&u->unpin&&
        port_user_pin_io_range(u->map,address,bytes,writing,u->physical,
            u->physical_context,u->pin,u->unpin,u->lock_context,locks)==
                PORT_USER_OK ? 1 : 0;
}
void port_exe_user_access_init(struct port_exe_access *access,
                              struct port_exe_user_access *context,
                              const struct port_user_map *map,
                              port_phys_page_resolver physical,void *opaque){
    if(!access||!context)return;
    *context=(struct port_exe_user_access){.map=map,.physical=physical,
        .physical_context=opaque};
    *access=(struct port_exe_access){context,user_read,user_write,
                                     user_can_read,user_can_write,0};
}
void port_exe_user_access_init_pinned(struct port_exe_access *access,
                              struct port_exe_user_access *context,
                              const struct port_user_map *map,
                              port_phys_page_resolver physical,void *opaque,
                              port_user_page_pin pin,
                              port_user_page_unpin unpin,void *lock_context){
    if(!access||!context)return;
    *context=(struct port_exe_user_access){.map=map,.physical=physical,
        .physical_context=opaque,.pin=pin,.unpin=unpin,
        .lock_context=lock_context};
    *access=(struct port_exe_access){context,user_read,user_write,
                                     user_can_read,user_can_write,user_lock_io};
}

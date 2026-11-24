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
void port_exe_user_access_init(struct port_exe_access *access,
                              struct port_exe_user_access *context,
                              const struct port_user_map *map,
                              port_phys_page_resolver physical,void *opaque){
    if(!access||!context)return;
    *context=(struct port_exe_user_access){map,physical,opaque};
    *access=(struct port_exe_access){context,user_read,user_write,
                                     user_can_read,user_can_write};
}

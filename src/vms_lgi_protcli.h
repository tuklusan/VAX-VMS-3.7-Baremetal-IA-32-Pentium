#ifndef VMS_LGI_PROTCLI_H
#define VMS_LGI_PROTCLI_H
#include <stdint.h>
#include "vms_exe_setprt.h"
/* LOGIN/SRC/PROTCLI.MAR: CLI PTE ownership and image handoff. */
struct port_lgi_protcli_ops {
    void *context;
    uint32_t (*lock_routine)(void *);
    uint32_t (*get_page)(void *,uint32_t,struct port_exe_page_state *);
    uint32_t (*set_page)(void *,const struct port_exe_page_state *);
    int (*user_writable)(void *,uint32_t);
    void (*invalidate)(void *,uint32_t);
    void (*lock_tables)(void *);
    void (*unlock_tables)(void *);
};
uint32_t port_lgi_protect_cli(const uint32_t image_range[2],
                              const struct port_lgi_protcli_ops *);
struct port_lgi_execute_ops {
    void *context;
    uint32_t (*rundown_user)(void *);
    uint32_t (*activate_image)(void *);
};
/* The activation callback normally transfers control and never returns. */
uint32_t port_lgi_execute_image(const struct port_lgi_execute_ops *);
#endif

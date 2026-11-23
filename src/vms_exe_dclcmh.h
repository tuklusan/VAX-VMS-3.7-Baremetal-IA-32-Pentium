#ifndef VMS_EXE_DCLCMH_H
#define VMS_EXE_DCLCMH_H
#include <stdint.h>
#include "vms_exe_access.h"
#include "vms_exe_gettim.h"
enum { PORT_SS_IVSSRQ=372u };
/* SYS/SRC/SYSDCLCMH.MAR: two change-mode vectors and one compat vector. */
struct port_exe_change_handlers {
    uint32_t supervisor,user,compatibility;
    uint8_t previous_mode;
};
struct port_exe_bad_service {
    void *context;
    /* Invalid inner-mode invocation takes the original bugcheck route. */
    void (*bugcheck)(void *);
};
uint32_t port_exe_dclcmh(struct port_exe_change_handlers *h,
                         uint32_t new_address,uint32_t old_address,
                         uint32_t type,const struct port_exe_access *user,
                         const struct port_exe_bad_service *bad);
#endif

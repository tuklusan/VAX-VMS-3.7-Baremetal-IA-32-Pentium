#ifndef VMS_EXE_ADJWSL_H
#define VMS_EXE_ADJWSL_H
#include "vms_exe_access.h"
#include "vms_exe_gettim.h"
/* SYS/SRC/SYSADJWSL.MAR: PHD working set size and WSL allocation. */
struct port_exe_working_set {
    uint16_t size,first_index,first_dynamic,last_index,extent;
    uint16_t min_ws,fluid,pt_max,pt_locked,extra_dynamic;
    uint16_t used_process,used_global,peak,image_peak;
    uint16_t peak_check,image_peak_check,image_counting;
    uint32_t physical_pages,free_limit,system_max_ws;
};
struct port_exe_adjwsl_ops {
    const struct port_exe_access *memory;
    void *context;
    /* Grows the header by actual WSLE entries; may return fewer than asked. */
    uint32_t (*expand_header)(void *,uint32_t,uint32_t *);
    /* Replaces a present WSL entry when free slots are exhausted. */
    uint32_t (*free_page)(void *);
    uint32_t (*wait_resource)(void *,uint32_t);
};
uint32_t port_exe_adjwsl(struct port_exe_working_set *w,int32_t pages,
                         uint32_t returned_limit,const struct port_exe_adjwsl_ops *ops);
void port_exe_wspchk(struct port_exe_working_set *w);
void port_exe_extra_dyn_ws(struct port_exe_working_set *w);
#endif

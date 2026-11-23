#ifndef VMS_EXE_GETPTI_H
#define VMS_EXE_GETPTI_H
#include <stdint.h>
#include "vms_exe_gettim.h"
#include "vms_exe_access.h"
enum { PORT_SS_LENVIO=396u, PORT_SS_PAGOWNVIO=492u,
       PORT_PTI_DEMAND_ZERO=1u, PORT_PTI_VAX_PAGE=512u };
/* Type values follow MMG$PAGETYPE in VMS 3.7. */
enum { PORT_PTI_TRANSITION=0u, PORT_PTI_PAGEFILE=1u,
       PORT_PTI_GLOBAL=2u, PORT_PTI_SECTION=3u, PORT_PTI_VALID=4u };
struct port_exe_pti_page {
    uint8_t type,global_type,global_valid,section_demand_zero;
    uint32_t frame_or_section;
};
struct port_exe_pti_request {
    uint32_t input_range,return_range,access_mode,mask,minimum_pages;
};
struct port_exe_pti_ops {
    const struct port_exe_access *user;
    void *context;
    /* Read an actual VMS PTE and test ownership at the supplied mode. */
    uint32_t (*get_page)(void *,uint32_t,uint8_t,struct port_exe_pti_page *);
};
uint32_t port_exe_getpti(uint8_t previous_mode,
                         const struct port_exe_pti_request *request,
                         const struct port_exe_pti_ops *ops);
#endif

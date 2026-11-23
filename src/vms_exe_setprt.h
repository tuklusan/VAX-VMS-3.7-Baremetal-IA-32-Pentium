#ifndef VMS_EXE_SETPRT_H
#define VMS_EXE_SETPRT_H
#include "vms_exe_access.h"
#include "vms_exe_gettim.h"
/* SYS/SRC/SYSSETPRT.MAR: per-page protection with VMS page ownership. */
enum {PORT_SS_IVPROTECT=756u,PORT_SS_PAGOWNVIO=492u,
      PORT_SS_LENVIO=396u,PORT_SS_EXQUOTA_M=28u,
      PORT_EXE_PTE_TRANSITION=0u,PORT_EXE_PTE_PAGEFILE=1u,
      PORT_EXE_PTE_GLOBAL=2u,PORT_EXE_PTE_SECTION=3u,
      PORT_EXE_PTE_VALID=4u};
struct port_exe_page_state {
    uint32_t virtual_address;
    uint8_t exists,owner_mode,protection,type;
    uint8_t global_crf,section_writable,window_mapping,shared_memory;
    uint8_t process_page,backed_section,backing_writable;
    uint8_t copy_on_reference,modified;
    uint32_t pfn;
};
struct port_exe_setprt_ops {
    void *context;
    const struct port_exe_access *memory;
    uint32_t (*get_page)(void *,uint32_t,struct port_exe_page_state *);
    uint32_t (*set_page)(void *,const struct port_exe_page_state *);
    uint32_t (*fault_page)(void *,uint32_t);
    uint32_t (*decrement_section)(void *,uint32_t);
    void (*invalidate_tlb)(void *,uint32_t);
    void (*lock_pages)(void *);
    void (*unlock_pages)(void *);
};
struct port_exe_setprt_request {
    uint32_t input_range,return_range,old_protection;
    uint32_t requested_mode,protection;
};
uint32_t port_exe_setprt(const struct port_exe_setprt_request *,
                         uint8_t previous_mode,int32_t *paging_quota,
                         const struct port_exe_setprt_ops *);
#endif

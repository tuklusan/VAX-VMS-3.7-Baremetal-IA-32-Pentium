#ifndef VMS_EXE_PURGWS_H
#define VMS_EXE_PURGWS_H
#include "vms_exe_access.h"
#include "vms_exe_gettim.h"
/* SYS/SRC/SYSPURGWS.MAR: working-set scan by 512-byte VMS pages. */
struct port_exe_ws_entry {
    uint32_t virtual_address;
    uint8_t valid,global_page,ws_locked,pfn_locked,owner_mode;
};
struct port_exe_purgws_ops {
    const struct port_exe_access *memory;
    void *context;
    struct port_exe_ws_entry *entries;
    uint32_t entry_count,locked_count;
    void (*lock_ws)(void *);
    void (*unlock_ws)(void *);
    uint32_t (*unlock_global)(void *,struct port_exe_ws_entry *);
    uint32_t (*free_entry)(void *,struct port_exe_ws_entry *);
    void (*fatal_bad_wsl)(void *);
};
uint32_t port_exe_purgws(uint32_t input_range,uint8_t previous_mode,
                         const struct port_exe_purgws_ops *,unsigned *removed);
uint32_t port_exe_purgws_scan(uint32_t start,uint32_t end,uint8_t mode,
                              uint8_t only_globals,
                              const struct port_exe_purgws_ops *,unsigned *removed);
#endif

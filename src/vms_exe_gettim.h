#ifndef VMS_EXE_GETTIM_H
#define VMS_EXE_GETTIM_H
#include <stdint.h>
#include "vms_userptr.h"

/* SYS/SRC/SYSGETTIM.MAR. Values use VMS 100-ns system-time units. */
enum { PORT_SS_NORMAL=1u, PORT_SS_ACCVIO=12u,
       PORT_SS_BADPARAM=20u, PORT_SS_NOPRIV=36u,
       PORT_SS_WASSET=9u, PORT_SS_ILLEFC=236u,
       PORT_SS_UNASEFC=564u, PORT_SS_BADQUEUEHDR=916u,
       PORT_SS_EXASTLM=10756u };
struct port_exe_systime {
    uint32_t sequence;
    uint32_t low;
    uint32_t high;
};
/* The clock's single writer publishes a complete pair of longwords. */
void port_exe_systime_store(struct port_exe_systime *clock,uint64_t time);
/* Single-writer IRQ0 clock step: one 10-ms tick in VMS units. */
void port_exe_systime_tick(struct port_exe_systime *clock);
/* The writer checks access under the calling mode and copies eight bytes. */
typedef int (*port_exe_time_write)(void *context,uint32_t address,
                                   const uint32_t value[2]);
uint32_t port_exe_gettim(const struct port_exe_systime *clock,
                         uint32_t address,port_exe_time_write write,
                         void *context);
/* User-mode access uses the current IA-32 page tables and a physical alias. */
struct port_exe_user_time {
    const struct port_user_map *map;
    port_phys_page_resolver physical;
    void *context;
};
uint32_t port_exe_gettim_user(const struct port_exe_systime *clock,
                              uint32_t address,
                              const struct port_exe_user_time *user);
#endif

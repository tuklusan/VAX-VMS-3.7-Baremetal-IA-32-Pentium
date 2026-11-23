#ifndef VMS_EXE_SETIME_H
#define VMS_EXE_SETIME_H
#include <stdint.h>
#include "vms_exe_gettim.h"
#include "vms_exe_schevt.h"
#include "vms_exe_access.h"
/* SYS/SRC/SYSSETIME.MAR: TODR, system time, timer queue and SYS.EXE. */
enum { PORT_EXE_TODR_BIAS=0x10000000u };
struct port_exe_time_state {
    struct port_exe_systime *clock;
    struct port_exe_timer_queue *timers;
    uint64_t base_time;
    uint32_t todr;
};
struct port_exe_setime_ops {
    void *context;
    const struct port_exe_access *memory;
    uint32_t (*read_todr)(void *,uint32_t *);
    uint32_t (*write_todr)(void *,uint32_t);
    uint32_t (*check_bootcb)(void *);
    /* Must write the genuine SYS.EXE SYSPARAM blocks via Files-11. */
    uint32_t (*save_sysparam)(void *,uint32_t,uint64_t);
    void (*lock_clock)(void *);
    void (*unlock_clock)(void *);
};
uint32_t port_exe_setime(struct port_exe_time_state *,uint32_t,
                         uint8_t oper_priv,uint8_t logio_priv,
                         const struct port_exe_setime_ops *);
#endif

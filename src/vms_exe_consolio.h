#ifndef VMS_EXE_CONSOLIO_H
#define VMS_EXE_CONSOLIO_H
#include <stdint.h>
/* SYS/SRC/CONSOLIO.MAR. All output is synchronous and uses a real UART. */
struct port_exe_console {
    void *context;
    int (*receive_ready)(void *);
    int (*receive)(void *,uint8_t *);
    int (*transmit_ready)(void *,uint32_t device);
    int (*transmit)(void *,uint32_t device,uint8_t byte);
};
/* The 16 original hexadecimal digits, in their original order. */
extern const char port_exe_hextab[16];
int port_exe_outchar(const struct port_exe_console *,uint32_t device,uint8_t byte);
int port_exe_outblank(const struct port_exe_console *,uint32_t device);
int port_exe_outcrlf(const struct port_exe_console *,uint32_t device);
int port_exe_outhex(const struct port_exe_console *,uint32_t device,uint32_t value);
int port_exe_outcstring(const struct port_exe_console *,uint32_t device,const uint8_t *counted);
int port_exe_outzstring(const struct port_exe_console *,uint32_t device,const char *zero_terminated);
#endif

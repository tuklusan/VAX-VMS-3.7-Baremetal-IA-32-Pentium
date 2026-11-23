#ifndef VMS_BOO_CONIO_H
#define VMS_BOO_CONIO_H
#include <stdint.h>
#include "vms_exe_gettim.h"
/* BOOTS/SRC/CONIO.MAR: console ASCIC read, editing, XON/XOFF. */
struct port_boo_console_ops {
    void *context;
    int (*available)(void *);
    int (*read)(void *,uint8_t *);
    int (*write)(void *,uint8_t);
};
uint32_t port_boo_conio_prompt(const char *prompt,uint32_t maximum,
                               uint8_t *counted_buffer,
                               const struct port_boo_console_ops *ops);
#endif

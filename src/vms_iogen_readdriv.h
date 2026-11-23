#ifndef VMS_IOGEN_READDRIV_H
#define VMS_IOGEN_READDRIV_H
#include <stdint.h>
#include "vms_exe_gettim.h"
/* BOOTS/SRC/READDRIV.MAR: open and map a device driver image. */
struct port_iogen_readdriv_ops {
    void *context;
    uint32_t (*open)(void *,const char *,uint16_t,uint16_t *);
    uint32_t (*map_section)(void *,uint16_t,uint32_t[2]);
    void (*signal_open_error)(void *,uint32_t);
};
enum { PORT_IOGEN_NOLOAD_DB=1u, PORT_IOGEN_NOSUPPORT=2u };
uint32_t port_iogen_readdriv(const uint8_t *ascic_name,uint8_t acf_flags,
                             uint16_t *channel,uint32_t range[2],
                             const struct port_iogen_readdriv_ops *);
#endif

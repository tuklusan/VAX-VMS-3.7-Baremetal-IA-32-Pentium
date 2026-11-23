#ifndef VMS_BOO_READPROMPT_H
#define VMS_BOO_READPROMPT_H
#include <stdint.h>
#include "vms_exe_gettim.h"
/* BOOTS/SRC/READPRMPT.MAR: standalone SYSGEN terminal QIO. */
struct port_boo_readprompt {
    void *context;
    uint16_t channel;
    uint8_t output[256];
    uint16_t output_length;
    uint32_t (*assign)(void *,const char *,uint16_t *channel);
    uint32_t (*qiow_readprompt)(void *,uint16_t,const char *,uint16_t,
                                  uint8_t *,uint16_t,uint16_t *iosb_count);
    uint32_t (*qiow_write)(void *,uint16_t,const uint8_t *,uint16_t);
};
uint32_t port_boo_readprompt(struct port_boo_readprompt *io,
                              const char *prompt,uint16_t size,uint8_t *counted);
uint32_t port_boo_output_line(struct port_boo_readprompt *io);
#endif

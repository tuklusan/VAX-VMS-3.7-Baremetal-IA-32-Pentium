#ifndef VMS_RTF_READLBN_H
#define VMS_RTF_READLBN_H
#include <stdint.h>
#include "vms_exe_gettim.h"
/* BOOTS/SRC/READLBN.MAR: RT-11 BOOTBLOCK reader through QIOW. */
struct port_rtf_target {uint8_t name[20];uint16_t name_size;};
struct port_rtf_read_ops {
    void *context;
    uint32_t (*assign)(void *,const uint8_t *,uint16_t,uint16_t *);
    uint32_t (*read_lbn)(void *,uint16_t,uint32_t,uint8_t[512],uint16_t *);
    uint32_t (*deassign)(void *,uint16_t);
};
uint32_t port_rtf_target_dev(struct port_rtf_target *,const uint8_t *,uint16_t);
uint32_t port_rtf_read_lbn(const struct port_rtf_target *,uint32_t,uint8_t[512],
                           const struct port_rtf_read_ops *);
#endif

#ifndef VMS_BOO_SYSGENMN_H
#define VMS_BOO_SYSGENMN_H
#include <stdint.h>
#include "vms_exe_gettim.h"
/* BOOTS/SRC/SYSGENMN.MAR, native standalone SYSGEN startup and PUTERROR. */
struct port_boo_sysgen {
    void *context;
    uint8_t request_status;
    uint8_t foreign_verb;
    uint16_t command_length;
    uint32_t command_address;
    uint32_t eof_status;
    uint32_t (*lock_image)(void *);
    uint32_t (*cli_utility)(void *,uint8_t *request_status);
    uint32_t (*use_active)(void *);
    uint32_t (*read_commands)(void *,uint8_t verify);
    uint32_t (*putmsg)(void *,uint32_t status,const char *facility);
};
uint32_t port_boo_sysgen_start(struct port_boo_sysgen *,uint32_t cli_flags);
uint32_t port_boo_sysgen_puterror(struct port_boo_sysgen *,uint32_t status,
                                  const char *facility);
#endif

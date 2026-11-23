#ifndef PORT_VMS_DCL_DESCR_H
#define PORT_VMS_DCL_DESCR_H
#include <stdint.h>
/* DCLDEF.MDL PTR: VALUE[7:0], OFFSET[19:8], FLAGS[23:20],
   TERM[27:24], TYPE[31:28]. */
struct port_dcl_descr_state {
    const uint32_t *next,*end;
    const char *command;
    unsigned length;
};
struct port_dcl_descr_result {
    unsigned value,flags,term,type;
    const char *item;
};
int port_dcl_getdval(struct port_dcl_descr_state *state,
                      struct port_dcl_descr_result *result);
#endif

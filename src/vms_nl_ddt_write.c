#include "vms_nl_ddt.h"
/* NLDRIVER.MAR 112-133 WRITE: return SS$_NORMAL to EXE$FINISHIOC. */
uint32_t port_v37_nl_fdt_write(struct port_v37_qio_request *r){
    return port_v37_qio_finishioc(r,PORT_SYS85_NORMAL);
}

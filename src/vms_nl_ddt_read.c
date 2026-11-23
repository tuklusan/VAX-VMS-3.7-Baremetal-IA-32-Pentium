#include "vms_nl_ddt.h"
/* NLDRIVER.MAR 112-133 READ: return SS$_ENDOFFILE to EXE$FINISHIOC. */
uint32_t port_v37_nl_fdt_read(struct port_v37_qio_request *r){
    return port_v37_qio_finishioc(r,PORT_SYS85_ENDOFFILE);
}

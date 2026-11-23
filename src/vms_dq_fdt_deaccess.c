#include "vms_dq_fdt.h"
/* Original DRIVER/SRC/DQDRIVER.MAR 566-568 FUNCTAB source class: DEACCESS.
 * Each bit position comes from VMSLIB/SRC/STARDEFFL.MDL (IO$_VIRTUAL=63).
 * No action address or driver is invented at this stage. */
uint64_t port_dq_fdt_deaccess(void){
    return PORT_DQ_BIT(52);
}

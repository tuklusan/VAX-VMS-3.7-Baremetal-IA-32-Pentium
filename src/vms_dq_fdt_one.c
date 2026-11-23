#include "vms_dq_fdt.h"
/* Original DRIVER/SRC/DQDRIVER.MAR 583-586 FUNCTAB source class: ONE.
 * Each bit position comes from VMSLIB/SRC/STARDEFFL.MDL (IO$_VIRTUAL=63).
 * No action address or driver is invented at this stage. */
uint64_t port_dq_fdt_one(void){
    return PORT_DQ_BIT(2);
}

#include "vms_dq_fdt.h"
/* Original DRIVER/SRC/DQDRIVER.MAR 546-553 FUNCTAB source class: READ.
 * Each bit position comes from VMSLIB/SRC/STARDEFFL.MDL (IO$_VIRTUAL=63).
 * No action address or driver is invented at this stage. */
uint64_t port_dq_fdt_read(void){
    return PORT_DQ_BIT(14) |
        PORT_DQ_BIT(33) |
        PORT_DQ_BIT(12) |
        PORT_DQ_BIT(49);
}

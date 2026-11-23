#include "vms_dq_fdt.h"
/* Original DRIVER/SRC/DQDRIVER.MAR 576-582 FUNCTAB source class: ZERO.
 * Each bit position comes from VMSLIB/SRC/STARDEFFL.MDL (IO$_VIRTUAL=63).
 * No action address or driver is invented at this stage. */
uint64_t port_dq_fdt_zero(void){
    return PORT_DQ_BIT(0) |
        PORT_DQ_BIT(1) |
        PORT_DQ_BIT(3) |
        PORT_DQ_BIT(4) |
        PORT_DQ_BIT(8) |
        PORT_DQ_BIT(17);
}

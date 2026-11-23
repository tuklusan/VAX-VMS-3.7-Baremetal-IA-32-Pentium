#include "vms_dq_fdt.h"
/* Original DRIVER/SRC/DQDRIVER.MAR 591-594 FUNCTAB source class: SET.
 * Each bit position comes from VMSLIB/SRC/STARDEFFL.MDL (IO$_VIRTUAL=63).
 * No action address or driver is invented at this stage. */
uint64_t port_dq_fdt_set(void){
    return PORT_DQ_BIT(26) |
        PORT_DQ_BIT(35);
}

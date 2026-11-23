#include "vms_dq_fdt.h"
/* Original DRIVER/SRC/DQDRIVER.MAR 569-572 FUNCTAB source class: MODIFY.
 * Each bit position comes from VMSLIB/SRC/STARDEFFL.MDL (IO$_VIRTUAL=63).
 * No action address or driver is invented at this stage. */
uint64_t port_dq_fdt_modify(void){
    return PORT_DQ_BIT(56) |
        PORT_DQ_BIT(53) |
        PORT_DQ_BIT(54);
}

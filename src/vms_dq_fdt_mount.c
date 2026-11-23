#include "vms_dq_fdt.h"
/* Original DRIVER/SRC/DQDRIVER.MAR 573-575 FUNCTAB source class: MOUNT.
 * Each bit position comes from VMSLIB/SRC/STARDEFFL.MDL (IO$_VIRTUAL=63).
 * No action address or driver is invented at this stage. */
uint64_t port_dq_fdt_mount(void){
    return PORT_DQ_BIT(57);
}

#include "vms_nl_ddt.h"
/* IOC$RETURN is a no-op for START, UNSOLIC, CANCEL, REGDUMP, UNITINIT,
 * ALTSTART in the original DDTAB defaults. It is not a physical device. */
uint32_t port_v37_nl_ioc_return(void){return PORT_SYS85_NORMAL;}

#include "vms_qio_post.h"
/* SYSQIOREQ.MAR 741-742: EXE$FINISHIOC clears R1 before the common
 * FINISHIO path. R0 is the final device status, not the service status. */
uint32_t port_v37_qio_finishioc(void *context,uint32_t status){
    return port_v37_qio_finishio(context,status,0u);
}

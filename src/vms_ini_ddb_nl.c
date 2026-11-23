#include "vms_ini_devicedb.h"
/* DEVICEDAT.MAR 457-459: NLA generic DDB terminates IOC$GL_DEVLIST. */
void port_v37_ddb_nl(uint8_t *d,uint32_t ucb){
    port_v37_ddb_base(d,0u,ucb,"NLA",3u,"NLDRIVER",8u);
}

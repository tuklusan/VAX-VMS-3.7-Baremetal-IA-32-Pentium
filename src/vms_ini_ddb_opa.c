#include "vms_ini_devicedb.h"
/* DEVICEDAT.MAR 233-234: console OPA generic DDB; class DDT not loaded. */
void port_v37_ddb_opa(uint8_t *d,uint32_t next,uint32_t ucb){
    port_v37_ddb_base(d,next,ucb,"OPA",3u,"OPERATOR",8u);
}

#include <assert.h>
#include <string.h>
#include "vms_dcl_on.h"
#include "vms_dcl_status.h"
static void reset(void *ctx){port_dcl_on_reset(ctx);}
int main(void){
 struct port_dcl_on_process p;
 port_dcl_on_init(&p,1,0);
 assert(p.on_level==2u&&!p.on_error[0]);
 assert(port_dcl_on(" ERROR THEN EXIT",&p)==PORT_DCL_ON_LEVEL);
 assert(!p.on_error[0]);
 p.indirect_depth=1;
 assert(port_dcl_on(" ERROR THEN SHOW STATUS",&p)==PORT_DCL_ON_NORMAL);
 assert(!strcmp(p.on_error,"SHOW STATUS")&&p.on_level==2u);
 assert(port_dcl_on(" SEVERE_ERROR THEN EXIT",&p)==PORT_DCL_ON_NORMAL);
 assert(p.on_level==4u&&!strcmp(p.on_error,"EXIT"));
 assert(port_dcl_on(" WARNING THEN SHOW TIME",&p)==PORT_DCL_ON_NORMAL);
 assert(p.on_level==0u);
 assert(port_dcl_on(" CONTROL_Y THEN CONTINUE",&p)==PORT_DCL_ON_NORMAL);
 assert(p.control_enabled==1u&&!strcmp(p.on_control_y,"CONTINUE"));
 assert(p.on_level==0u&&!strcmp(p.on_error,"SHOW TIME"));
 port_dcl_on_ctly_reset(&p);
 assert(!p.on_control_y[0]&&!p.control_enabled&&!strcmp(p.on_error,"SHOW TIME"));
 assert(port_dcl_on(" ERROR THEN EXIT",&p)==PORT_DCL_ON_NORMAL);
 assert(p.on_level==2u&&!strcmp(p.on_error,"EXIT"));
 struct port_dcl_status_process ps={.on_level=(uint8_t)p.on_level,
                                     .on_error=p.on_error};
 struct port_dcl_status_work w={0};
 const struct port_dcl_status_ops ops={.on_reset=reset};
 assert(port_dcl_set_status(&ps,&w,2u,&ops,&p)==1);
 assert(!strcmp(w.command,"EXIT")&&p.on_level==2u&&!p.on_error[0]);
 assert(port_dcl_on(" BAD THEN EXIT",&p)==PORT_DCL_ON_BADLEVEL);
 assert(port_dcl_on(" ERROR ELSE EXIT",&p)==PORT_DCL_ON_BADCOMMAND);
 assert(port_dcl_on("ERROR THEN EXIT",&p)==PORT_DCL_ON_DELIMITER);
 assert(port_dcl_on(" ERROR THEN ",&p)==PORT_DCL_ON_BADCOMMAND);
 port_dcl_on_reset(&p);
 assert(!p.on_error[0]&&p.on_level==2u);
 return 0;
}

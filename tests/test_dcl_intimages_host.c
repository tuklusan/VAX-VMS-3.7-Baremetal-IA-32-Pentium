#include <assert.h>
#include <string.h>
#include "vms_dcl_intimages.h"
int main(void){
 assert(PORT_DCL_IMAGE_ALLOCATE==0x80u);
 assert(PORT_DCL_IMAGE_EOJ==0x80u+42u);
 for(unsigned i=0;i<PORT_DCL_IMAGE_COUNT;i++){
    const char *s=port_dcl_image_name(0x80u+i);
    assert(s&&port_dcl_image_lookup(s)==0x80u+i);
 }
 assert(port_dcl_image_lookup("inquire")==PORT_DCL_IMAGE_INQUIRE);
 assert(port_dcl_image_lookup("logout")==PORT_DCL_IMAGE_LOGOUT);
 assert(port_dcl_image_lookup("NONEXISTENT")==0);
 assert(port_dcl_image_name(0)==0 && port_dcl_image_name(0xff)==0);
 assert(!strcmp(port_dcl_image_name(PORT_DCL_IMAGE_SHOWSYMBL),"SHOWSYMBL"));
 return 0;
}

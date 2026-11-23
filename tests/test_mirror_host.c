#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_mirror.h"
int main(void) {
    const uint8_t token[]={'P','I','N','G',0x00,0xff};
    uint8_t req[PORT_MIRROR_MAX],reply[PORT_MIRROR_MAX];
    int n=port_mirror_make_request(req,sizeof req,token,sizeof token);
    assert(n==7&&req[0]==0&&memcmp(req+1,token,sizeof token)==0);
    int rn=port_mirror_reply(reply,sizeof reply,req,(unsigned)n);
    assert(rn==n&&reply[0]==1&&memcmp(reply+1,token,sizeof token)==0);
    assert(port_mirror_verify(req,n,reply,rn)==0);
    reply[3]^=0x01;
    assert(port_mirror_verify(req,n,reply,rn)<0);
    reply[3]^=0x01;
    assert(port_mirror_verify(req,n,reply,rn-1)<0);
    uint8_t unsupported[]={3,0,1};
    assert(port_mirror_reply(reply,sizeof reply,unsupported,sizeof unsupported)==1);
    assert(reply[0]==0xff);
    assert(port_mirror_verify(req,n,reply,1)==-2);
    assert(port_mirror_reply(reply,0,unsupported,sizeof unsupported)<0);
    assert(port_mirror_reply(reply,1,req,(unsigned)n)<0);
    assert(port_mirror_make_request(req,3,token,6)<0);
    assert(port_mirror_make_request(req,sizeof req,0,1)<0);
    assert(port_mirror_verify(0,1,reply,1)<0);
    assert(port_mirror_reply(reply,sizeof reply,req,0)<0);
    assert(port_mirror_make_request(req,sizeof req,0,0)==1);
    assert(port_mirror_reply(reply,sizeof reply,req,1)==1);
    assert(port_mirror_verify(req,1,reply,1)==0);
    puts("M21 DECnet object-25 Mirror protocol codec PASS");
    return 0;
}

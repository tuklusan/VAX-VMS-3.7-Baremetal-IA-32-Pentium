/* Deterministic adversarial wire input test; not a proof of security. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_decnet.h"
#include "vms_nsp.h"
#include "vms_nsp_data.h"
#include "vms_session.h"
#include "vms_mirror.h"
static uint32_t state=0x3737f00du;
static uint32_t rng(void) {
    uint32_t x=state;x^=x<<13;x^=x>>17;x^=x<<5;return state=x;
}
int main(void) {
    uint8_t raw[1536],reply[600];
    struct port_decnet_short shortmsg;
    struct port_decnet_hello hello;
    struct port_decnet_router_hello router;
    struct port_nsp_message ctrl;
    struct port_nsp_data data;
    struct port_nsp_data_ack ack;
    uint8_t object=0;
    unsigned valid=0;
    for(unsigned i=0;i<15000;i++) {
        unsigned n=rng()%sizeof raw;
        for(unsigned j=0;j<n;j++)raw[j]=(uint8_t)rng();
        (void)port_decnet_parse_short(raw,n,&shortmsg);
        (void)port_decnet_parse_router_hello(raw,n,&router);
        (void)port_decnet_parse_endnode_hello(raw,n,&hello);
        (void)port_nsp_parse(raw,n,&ctrl);
        (void)port_nsp_data_parse(raw,n,&data);
        (void)port_nsp_data_ack_parse(raw,n,&ack);
        (void)port_session_parse_request(raw,n,&object);
        if(n<600) {
            (void)port_mirror_reply(reply,sizeof reply,raw,n);
            (void)port_mirror_verify(raw,n,reply,n);
        }
    }
    /* Controlled one-bit mutations of valid routed NSP DATA fixtures. */
    const uint8_t user[]={0,2,'O','K'};
    uint8_t segment[100];
    int sn=port_nsp_data_build(segment,sizeof segment,0x60,0x5757,
                               0x3737,0,0,1,user,sizeof user);
    assert(sn>0);
    int flen=port_decnet_build_short(raw,sizeof raw,0x0401,0x0402,
                                     segment,(unsigned)sn);
    assert(flen>0);
    uint8_t baseline[1536];memcpy(baseline,raw,flen);
    for(unsigned i=0;i<15000;i++) {
        memcpy(raw,baseline,flen);
        unsigned byte=rng()%(unsigned)flen;
        raw[byte]^=(uint8_t)(1u<<(rng()%8u));
        if(port_decnet_parse_short(raw,(unsigned)flen,&shortmsg)==0) {
            ++valid;
            (void)port_nsp_data_parse(shortmsg.payload,
                                      shortmsg.payload_len,&data);
        }
    }
    assert(valid>0);
    puts("M24 deterministic fuzz: 30,000 malformed/mutated wire frames PASS");
    return 0;
}

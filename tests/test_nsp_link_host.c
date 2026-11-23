#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "vms_nsp_link.h"
int main(void) {
    struct port_nsp_link link={0};struct port_nsp_message m={0};
    assert(port_nsp_link_open(&link,0x401,0x402,0x1234,100)==0);
    assert(link.state==PORT_NSP_CI_SENT&&link.attempts==1);
    m.flags=PORT_NSP_CA;m.destination_link=0x1234;
    assert(port_nsp_link_rx(&link,0x403,0x401,&m)<0);
    assert(link.rejected==1&&link.state==PORT_NSP_CI_SENT);
    assert(port_nsp_link_rx(&link,0x402,0x401,&m)==0);
    assert(link.state==PORT_NSP_ACKED&&link.received==1);
    m.flags=PORT_NSP_CC;m.source_link=0x5678;
    assert(port_nsp_link_rx(&link,0x402,0x401,&m)==0);
    assert(link.state==PORT_NSP_CONNECTED&&link.remote_link==0x5678);
    assert(port_nsp_link_tick(&link,30000)==0);
    assert(port_nsp_link_disconnect(&link,30000)==0);
    m.flags=PORT_NSP_DC;m.reason=42;
    assert(port_nsp_link_rx(&link,0x402,0x401,&m)==0);
    assert(link.state==PORT_NSP_CLOSED&&link.last_reason==42);
    assert(port_nsp_link_open(&link,0x401,0x402,0x1234,100)==0);
    assert(port_nsp_link_tick(&link,399)==0);
    assert(port_nsp_link_tick(&link,400)==1&&link.attempts==2);
    assert(port_nsp_link_tick(&link,699)==0);
    assert(port_nsp_link_tick(&link,700)==1&&link.attempts==3);
    assert(port_nsp_link_tick(&link,1000)==-1);
    assert(link.state==PORT_NSP_TIMED_OUT&&link.timeouts==1);
    assert(port_nsp_link_tick(&link,2000)==0);
    assert(port_nsp_link_open(&link,0x401,0x402,0x2222,UINT32_MAX-20)==0);
    assert(port_nsp_link_tick(&link,279)==1); /* wrap-safe unsigned tick math */
    m.flags=PORT_NSP_DI;m.destination_link=0x2222;m.source_link=0;
    m.reason=4;
    assert(port_nsp_link_rx(&link,0x402,0x401,&m)==0);
    assert(link.state==PORT_NSP_REFUSED&&link.last_reason==4);
    assert(port_nsp_link_open(&link,0x401,0x401,0x1234,0)<0);
    port_nsp_link_reset(&link);
    assert(link.state==PORT_NSP_CLOSED&&link.received==0);
    puts("M17 NSP link handshake, peer validation, retry timeout PASS");
    return 0;
}

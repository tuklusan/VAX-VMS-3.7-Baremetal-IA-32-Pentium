#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "vms_decnet.h"
int main(void) {
    uint8_t f[1518]={0};
    int n=port_decnet_build_endnode_hello(f,sizeof f,0x401,576,15);
    assert(n==50);
    assert(f[0]==0xab&&f[3]==3&&f[4]==0);
    assert(f[6]==0xaa&&f[10]==1&&f[11]==4);
    assert(f[12]==0x60&&f[13]==3&&f[14]==34&&f[15]==0);
    assert(f[16]==0x0d&&f[17]==2&&f[18]==0&&f[19]==0);
    assert(f[20]==0xaa&&f[24]==1&&f[25]==4);
    assert(f[26]==3&&f[27]==0x40&&f[28]==2);
    for(unsigned i=30;i<38;i++)assert(f[i]==0);
    assert(f[38]==0xaa&&f[40]==4);
    assert(f[44]==15&&f[45]==0&&f[47]==2&&f[48]==0xaa&&f[49]==0xaa);
    struct port_decnet_hello h={0};
    assert(port_decnet_parse_endnode_hello(f,n,&h)==0);
    assert(h.nodeid==0x401&&h.blocksize==576&&h.timer==15);
    assert(h.neighbor_mac[0]==0xaa&&h.neighbor_mac[2]==4);
    assert(port_decnet_parse_endnode_hello(f,60,&h)==0); /* min frame pad */
    f[14]=0x35;assert(port_decnet_parse_endnode_hello(f,60,&h)!=0);f[14]=34;
    f[16]=0x0b;assert(port_decnet_parse_endnode_hello(f,60,&h)!=0);f[16]=0x0d;
    f[24]=2;assert(port_decnet_parse_endnode_hello(f,60,&h)!=0);f[24]=1;
    f[47]=3;assert(port_decnet_parse_endnode_hello(f,60,&h)!=0);f[47]=2;
    assert(port_decnet_build_endnode_hello(f,49,0x401,576,15)<0);
    assert(port_decnet_build_endnode_hello(f,sizeof f,0x401,0,15)<0);
    assert(port_decnet_build_endnode_hello(f,sizeof f,0x401,576,0)<0);
    puts("M14 DECnet endnode Hello host PASS: 34-byte field map, short length, errors");
    return 0;
}

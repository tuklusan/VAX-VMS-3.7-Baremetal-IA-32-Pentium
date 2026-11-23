#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "vms_decnet.h"
int main(void) {
    uint16_t local,other;
    assert(port_decnet_node(1,1,&local)==0&&local==0x401);
    assert(port_decnet_node(1,2,&other)==0&&other==0x402);
    assert(port_decnet_node(63,1023,&other)==0&&other==65535);
    assert(port_decnet_node(0,1,&local)==PORT_DECNET_INVALID);
    assert(port_decnet_node(1,0,&local)==PORT_DECNET_INVALID);
    assert(port_decnet_node(1,1024,&local)==PORT_DECNET_INVALID);
    uint8_t mac[6],exp[]={0xaa,0,4,0,1,4};
    port_decnet_node_mac(0x401,mac);
    assert(!memcmp(mac,exp,6));
    uint8_t f[1518]={0};const uint8_t payload[]={0x10,0x01,0xab};
    int n=port_decnet_build_short(f,sizeof f,0x401,0x402,payload,sizeof payload);
    assert(n==25);
    assert(f[0]==0xaa&&f[4]==2&&f[5]==4);
    assert(f[12]==0x60&&f[13]==3);
    assert(f[14]==9&&f[15]==0&&f[16]==2);
    assert(f[17]==2&&f[18]==4&&f[19]==1&&f[20]==4);
    assert(f[21]==0&&!memcmp(f+22,payload,sizeof payload));
    struct port_decnet_short packet={0};
    assert(port_decnet_parse_short(f,n,&packet)==0);
    assert(packet.dst_node==0x402&&packet.src_node==0x401&&packet.visits==0);
    assert(packet.payload_len==sizeof payload&&!memcmp(packet.payload,payload,sizeof payload));
    /* Minimum Ethernet frame padding is not part of the DECnet packet. */
    assert(port_decnet_parse_short(f,60,&packet)==0&&packet.payload_len==3);
    assert(port_decnet_parse_short(f,22,&packet)==PORT_DECNET_FORMAT);
    f[16]=6;assert(port_decnet_parse_short(f,n,&packet)==PORT_DECNET_UNSUPPORTED);
    f[16]=2;f[13]=0;assert(port_decnet_parse_short(f,n,&packet)==PORT_DECNET_FORMAT);
    f[13]=3;f[4]=3;assert(port_decnet_parse_short(f,n,&packet)==PORT_DECNET_OK);
    f[4]=2;f[14]=0xff;f[15]=0xff;
    assert(port_decnet_parse_short(f,n,&packet)==PORT_DECNET_FORMAT);
    assert(port_decnet_build_short(f,23,0x401,0x402,payload,3)==PORT_DECNET_INVALID);
    assert(port_decnet_build_short(f,sizeof f,0x401,0x402,0,1)==PORT_DECNET_INVALID);
    assert(port_decnet_build_short(f,sizeof f,0x401,0,payload,3)==PORT_DECNET_INVALID);
    puts("M13 DECnet host PASS: address, EtherType 6003, length, short route, padding");
    return 0;
}

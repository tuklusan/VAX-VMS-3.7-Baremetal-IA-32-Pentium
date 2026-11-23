#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "vms_decnet.h"
static void mkrouter(uint8_t f[1518],uint16_t node,uint8_t priority,
                     uint16_t timer,uint8_t which_group,unsigned n) {
    memset(f,0,1518);
    f[0]=0xab;f[3]=which_group;
    port_decnet_node_mac(node,f+6);
    f[12]=0x60;f[13]=0x03;
    f[14]=(uint8_t)(27+7*n);
    f[15]=0;
    uint8_t *p=f+16;
    p[0]=0x0b;p[1]=2;
    port_decnet_node_mac(node,p+4);
    p[10]=2; /* L1 router */
    p[11]=0xc4;p[12]=0x05; /* 1476 */
    p[13]=priority;p[14]=0;
    p[15]=(uint8_t)timer;p[16]=(uint8_t)(timer>>8);
    p[17]=0;
    p[18]=(uint8_t)(8+7*n);
    p[26]=(uint8_t)(7*n);
    if(n) {p[27]=0xaa;p[29]=4;}
}
int main(void) {
    uint8_t f[1518];struct port_decnet_router_hello h={0};
    struct port_decnet_adjacencies adj={0};
    mkrouter(f,0x0402,60,15,3,1);
    assert(port_decnet_parse_router_hello(f,50,&h)==0);
    assert(h.nodeid==0x402&&h.priority==60&&h.timer==15&&h.blocksize==1476);
    assert(h.list_count==1&&h.mac[4]==2);
    assert(port_decnet_adj_update(&adj,&h,100)==0);
    assert(adj.learned==1);
    const struct port_decnet_router_entry *best=port_decnet_adj_best(&adj,0x401);
    assert(best&&best->hello.nodeid==0x402);
    uint8_t hello[100]={0};
    assert(port_decnet_build_endnode_hello_to(hello,sizeof hello,0x401,576,15,
                                               best->hello.mac)==50);
    assert(hello[38]==0xaa&&hello[42]==2&&hello[43]==4);
    mkrouter(f,0x0403,60,15,4,0);
    assert(port_decnet_parse_router_hello(f,43,&h)==0);
    assert(port_decnet_adj_update(&adj,&h,120)==0);
    best=port_decnet_adj_best(&adj,0x401);
    assert(best&&best->hello.nodeid==0x403); /* tie: larger node */
    mkrouter(f,0x0404,90,15,3,0);
    assert(port_decnet_parse_router_hello(f,43,&h)==0);
    assert(port_decnet_adj_update(&adj,&h,120)==0);
    assert(port_decnet_adj_best(&adj,0x401)->hello.nodeid==0x404);
    mkrouter(f,0x0801,120,15,3,0);
    assert(port_decnet_parse_router_hello(f,43,&h)==0);
    assert(port_decnet_adj_update(&adj,&h,120)==0);
    assert(port_decnet_adj_best(&adj,0x401)->hello.nodeid==0x404);/* ignore other area */
    port_decnet_adj_expire(&adj,4599);
    assert(adj.expired==0);
    port_decnet_adj_expire(&adj,4620);
    assert(adj.expired==4&&port_decnet_adj_best(&adj,0x401)==0);
    port_decnet_adj_expire(&adj,4620);
    assert(adj.expired==4);
    mkrouter(f,0x0402,60,15,3,1);
    f[16]=0x0d;assert(port_decnet_parse_router_hello(f,50,&h)!=0);
    f[16]=0x0b;f[42]=2;assert(port_decnet_parse_router_hello(f,50,&h)!=0);
    f[42]=7;f[34]=0;assert(port_decnet_parse_router_hello(f,50,&h)!=0);
    puts("M15 router Hello PASS: canonical parse, counts, elected router, expiry");
    return 0;
}

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_sysboot_pktmem.h"
#define CHECK(x) do { if(!(x)){fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);return 1;}} while(0)
int main(void){
    struct port_boo_packet_settings s={
        .top_address=0x350000u,
        .irp_length=158u,.irp_count=4u,.irp_virtual_count=10u,
        .lrp_requested_size=511u,.lrp_count=2u,.lrp_virtual_count=5u,
        .srp_requested_size=72u,.srp_count=7u,.srp_virtual_count=9u,
        .nonpaged_dynamic_bytes=4900u,.nonpaged_virtual_bytes=6500u,
        .paged_dynamic_bytes=1700u};
    struct port_boo_packet_layout b={0};
    struct port_boo_parameters p={0};
    CHECK(port_boo_packet_layout(&s,&b,&p)==PORT_BOO_PACKET_OK);
    CHECK(b.srp.packet_bytes==80u && b.irp.packet_bytes==160u);
    CHECK(b.lrp.packet_bytes==560u);/* (511 + 64) & ~15 */
    CHECK(b.srp.first_address==s.top_address-1024u);
    CHECK(b.srp.resident_end==b.srp.first_address+1024u);
    CHECK(b.irp.reserved_end==b.srp.first_address);
    CHECK(b.irp.resident_end==b.irp.first_address+1024u);
    CHECK(b.irp.reserved_end-b.irp.first_address==2048u);
    CHECK(b.lrp.reserved_end==b.irp.first_address);
    CHECK(b.lrp.resident_end==b.lrp.first_address+1536u);
    CHECK(b.lrp.reserved_end-b.lrp.first_address==3072u);
    CHECK(b.nonpaged_end==b.nonpaged_start+4608u);
    CHECK(b.nonpaged_virtual_end==b.lrp.first_address);
    CHECK(b.nonpaged_virtual_end-b.nonpaged_start==6144u);
    CHECK(b.paged_dynamic_start==b.nonpaged_start-1536u);
    CHECK(p.irp_split_address==b.irp.first_address && p.irp_count==4u);
    CHECK(p.lrp_size==560u && p.lrp_count==2u);
    CHECK(p.lrp_minimum==448u && b.lrp_minimum==448u);
    CHECK(p.lrp_split_address==b.lrp.first_address);
    CHECK(p.srp_count==7u && p.srp_split_address==b.srp.first_address);
    CHECK(p.nonpaged_dynamic_bytes==4608u);
    struct port_boo_packet_layout saved=b;
    struct port_boo_parameters saved_p=p;
    s.irp_virtual_count=3u;
    CHECK(port_boo_packet_layout(&s,&b,&p)==PORT_BOO_PACKET_NOMEM);
    CHECK(memcmp(&b,&saved,sizeof b)==0 && memcmp(&p,&saved_p,sizeof p)==0);
    s.irp_virtual_count=10u;s.top_address=0x2000u;
    CHECK(port_boo_packet_layout(&s,&b,&p)==PORT_BOO_PACKET_NOMEM);
    CHECK(memcmp(&b,&saved,sizeof b)==0);
    s.top_address=0x350001u;
    CHECK(port_boo_packet_layout(&s,&b,&p)==PORT_BOO_PACKET_BADPARAM);
    s.top_address=0x350000u;s.irp_length=UINT32_MAX;
    CHECK(port_boo_packet_layout(&s,&b,&p)==PORT_BOO_PACKET_BADPARAM);
    s.irp_length=158u;s.srp_count=UINT32_MAX;
    CHECK(port_boo_packet_layout(&s,&b,&p)==PORT_BOO_PACKET_NOMEM);
    s.srp_count=7u;s.srp_virtual_count=9u;
    s.lrp_requested_size=UINT16_MAX;
    CHECK(port_boo_packet_layout(&s,&b,&p)==PORT_BOO_PACKET_BADPARAM);
    CHECK(memcmp(&b,&saved,sizeof b)==0);
    s.lrp_requested_size=511u;
    s.nonpaged_dynamic_bytes=100u;s.nonpaged_virtual_bytes=100u;
    CHECK(port_boo_packet_layout(&s,&b,&p)==PORT_BOO_PACKET_OK);
    CHECK(p.nonpaged_dynamic_bytes==0u);
    puts("original SYSBOOT 512-page/16-packet IRP-LRP-SRP virtual and resident allocation PASS");
    return 0;
}

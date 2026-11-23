#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_sysboot_map.h"
static int failures;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);failures++;}}while(0)
int main(void){
    const struct port_f11_extent map[3]={{2u,200u},{3u,500u},{1u,900u}};
    struct port_f11_extent submap[3];
    uint32_t lbn=0,remaining=0,next=0,used=0;
    CHECK(port_sysboot_mapvbn(map,3u,1u,&lbn,&remaining,&next)==PORT_F11_HOME_OK);
    CHECK(lbn==200u && remaining==2u && next==1u);
    CHECK(port_sysboot_mapvbn(map,3u,2u,&lbn,&remaining,&next)==PORT_F11_HOME_OK);
    CHECK(lbn==201u && remaining==1u);
    CHECK(port_sysboot_mapvbn(map,3u,3u,&lbn,&remaining,&next)==PORT_F11_HOME_OK);
    CHECK(lbn==500u && remaining==3u && next==2u);
    CHECK(port_sysboot_mapvbn(map,3u,6u,&lbn,&remaining,&next)==PORT_F11_HOME_OK);
    CHECK(lbn==900u && remaining==1u);
    CHECK(port_sysboot_mapvbn(map,3u,7u,&lbn,&remaining,&next)==PORT_F11_FILE_EOF);
    CHECK(port_sysboot_setmap(map,3u,2u,4u,submap,3u,&used)==PORT_F11_HOME_OK);
    CHECK(used==2u && submap[0].blocks==1u && submap[0].lbn==201u);
    CHECK(submap[1].blocks==3u && submap[1].lbn==500u);
    CHECK(port_sysboot_setmap(map,3u,3u,4u,submap,1u,&used)==PORT_F11_FILE_BAD_MAP);
    CHECK(port_sysboot_setmap(map,3u,6u,2u,submap,3u,&used)==PORT_F11_FILE_EOF);
    CHECK(port_sysboot_setmap(map,3u,3u,2u,submap,3u,&used)==PORT_F11_HOME_OK);
    CHECK(used==1u && submap[0].lbn==500u && submap[0].blocks==2u);
    const struct port_f11_extent invalid[1]={{2u,0xffffffffu}};
    CHECK(port_sysboot_mapvbn(invalid,1u,1u,&lbn,&remaining,&next)==PORT_F11_FILE_BAD_MAP);
    uint8_t header[512];memset(header,0,sizeof header);
    header[7]=2;header[1]=65;header[58]=6;
    header[130]=1;header[131]=0x40;header[132]=200;header[133]=0;
    header[134]=0;header[135]=0x80;header[136]=0xe8;header[137]=3;
    header[138]=0;header[139]=0;
    struct port_f11_extent actual[4];
    CHECK(port_f11_collect_extents(header,actual,4u,&used)==PORT_F11_HOME_OK);
    CHECK(used==2u && actual[0].blocks==2u && actual[0].lbn==200u);
    CHECK(actual[1].blocks==1u && actual[1].lbn==1000u);
    CHECK(port_f11_collect_extents(header,actual,1u,&used)==PORT_F11_FILE_BAD_MAP);
    header[58]=4u;
    CHECK(port_f11_collect_extents(header,actual,4u,&used)==PORT_F11_FILE_BAD_MAP);
    printf("SYSBOOT normalized extents, map VBN and segment map: %s\n",failures?"FAIL":"PASS");
    return !!failures;
}

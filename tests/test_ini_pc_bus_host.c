#include <stdio.h>
#include "vms_ini_pc_bus.h"
#include "vms_iniloa_iomap.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
struct fake {int ata_ok,uart_ok;unsigned reads;};
static int disk(void *ctx,uint32_t lba,uint16_t sectors[256]){
    struct fake *f=(struct fake *)ctx;
    f->reads++;
    if(lba!=0u)return -1;
    for(unsigned i=0;i<256;i++)sectors[i]=0u;
    return f->ata_ok?0:-1;
}
static int uart(void *ctx){return ((struct fake *)ctx)->uart_ok;}
int main(void){
    struct port_ini_io_descriptor d[2];
    CHECK(port_ini_pc_describe(0x1f0u,0x3f8u,d)==2u);
    CHECK(port_ini_pc_describe(0x170u,0x3f8u,d)==0u);
    CHECK(port_ini_pc_describe(0x1f0u,0x2f8u,d)==0u);
    CHECK(port_ini_pc_describe(0x1f0u,0u,d)==1u);
    CHECK(port_ini_pc_describe(0x1f0u,0x3f8u,d)==2u);
    struct fake f={1,1,0};
    struct port_ini_pc_bus b={.context=&f,.ata_read_sector=disk,
        .uart_ready=uart,.ata_io_port=0x1f0u,.uart_io_port=0x3f8u};
    const struct port_ini_io_ops ops={&b,port_ini_pc_probe,port_ini_pc_register};
    struct port_ini_io_state state={0};
    CHECK(port_ini_iomap(d,2u,0x1f0u,&ops,&state)==PORT_INI_MAP_OK);
    CHECK(state.num_nexus==2u && state.boot.boot_adapter_number==0u);
    CHECK(state.confreg[0]==PORT_INI_PC_ATA_PIO && state.sbiconf[0]==0x1f0u);
    CHECK(state.confreg[1]==PORT_INI_PC_16550_UART && state.sbiconf[1]==0x3f8u);
    CHECK(b.registered_mask==3u && b.probe_count==2u && b.register_count==2u);
    CHECK(f.reads==1u);
    CHECK(port_ini_pc_register(&b,&d[0])==0u);/* no duplicated driver */
    f.ata_ok=0;
    b=(struct port_ini_pc_bus){.context=&f,.ata_read_sector=disk,
        .uart_ready=uart,.ata_io_port=0x1f0u,.uart_io_port=0x3f8u};
    CHECK(port_ini_iomap(d,2u,0x1f0u,&ops,&state)==PORT_INI_MAP_NO_BOOT_DEVICE);
    CHECK(b.registered_mask==0u);
    f.ata_ok=1;f.uart_ok=0;
    CHECK(port_ini_iomap(d,2u,0x1f0u,&ops,&state)==PORT_INI_MAP_OK);
    CHECK(state.num_nexus==1u&&b.registered_mask==1u);
    CHECK(!state.confreg[1]&&!state.sbiconf[1]);
    puts("PC INITADP native physical ATA/COM1 adapter registration PASS");
    return 0;
}

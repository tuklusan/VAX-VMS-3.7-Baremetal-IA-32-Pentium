#include "vms_sysmount.h"
#include "vms_boot_files11.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct diskcontext {
    FILE *disk;
    struct boo_io_driver driver;
    struct port_f11_volume volume;
    uint32_t reads, mounted;
};
static uint32_t read_block(void *ctx,uint32_t mode,uint32_t fn,
                           uint32_t lbn,uint32_t count,void *buf){
    struct diskcontext *dc=ctx;
    if(mode||fn!=1u||count!=512u||!buf||
       fseek(dc->disk,(long)lbn*512L,SEEK_SET)!=0)return BOO_IO_FAILED;
    dc->reads++;
    return fread(buf,1,512,dc->disk)==512u?BOO_IO_NORMAL:BOO_IO_FAILED;
}
static uint32_t getchn(void *ctx,uint32_t channel,
                       struct port_sysmount_device *one,
                       struct port_sysmount_device *two){
    (void)ctx;
    if(channel!=1u)return 2;
    *one=(struct port_sysmount_device){0};
    memcpy(one->name,"IDE",4);
    one->flags=PORT_SYSMOUNT_FOD|PORT_SYSMOUNT_AVL;
    one->sector_bytes=512u;
    one->physical_blocks=8064u;
    *two=*one;
    return 1;
}
static uint32_t home(void *ctx,uint32_t channel,const char *name,struct port_sysmount_state *state){
    struct diskcontext *dc=ctx;
    uint8_t *block=state->home_block;
    struct port_f11_home info;
    if(channel!=1u||strcmp(name,"SYSTEMDISK")!=0||
       !(boo_rdwr_lbn(&dc->driver,1u,block,1u,512u)&1u))return 2;
    uint32_t code=port_f11_home_read(block,512u,1u,&info);
    if(code==PORT_F11_HOME_OK){state->level2=1;state->home_block_lbn=1u;}
    return code;
}
static uint32_t mount_ods2(void *ctx,struct port_sysmount_state *state){
    struct diskcontext *dc=ctx;
    uint32_t code=port_f11_mount(&dc->driver,1u,&dc->volume);
    if(code!=PORT_F11_HOME_OK)return code;
    dc->mounted++;
    state->volume_mounted=1;
    /* Real F11ACP has not started: mount must report incomplete. */
    state->acp_started=0;
    return 1u;
}
static uint32_t mount_ods1(void *ctx,struct port_sysmount_state *state){
    (void)ctx;(void)state;
    return 2;
}
int main(int argc,char **argv){
    if(argc!=2){fprintf(stderr,"Usage: %s portdisk.img\n",argv[0]);return 2;}
    struct diskcontext dc={0};
    dc.disk=fopen(argv[1],"rb");
    if(!dc.disk){perror(argv[1]);return 1;}
    dc.driver.transfer=read_block;
    dc.driver.ctx=&dc;
    struct port_sysmount_services services={0};
    services.getchn=getchn;
    services.read_home=home;
    services.mount_disk1=mount_ods1;
    services.mount_disk2=mount_ods2;
    services.context=&dc;
    struct port_sysmount_status codes={10u,12u,14u,16u,28u,1u};
    struct port_sysmount_state state;
    uint32_t rc=port_sysmount_system(&state,&services,&codes,1u);
    fclose(dc.disk);
    if(rc!=PORT_SYSMOUNT_INCOMPLETE||dc.mounted!=1u||dc.reads<2u||
       !state.volume_mounted||state.acp_started||
       !(state.mount_options&((uint64_t)1u<<PORT_MOUNT_OPT_IS_FILES11B))){
        fprintf(stderr,"FAIL: SYSINIT must read real Files-11 disk but block without F11ACP\n");
        return 1;
    }
    puts("PASS: real Files-11 disk mounted through SYSMOU; absent F11ACP prevents startup");
    return 0;
}

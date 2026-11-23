#include "vms_sysmount.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>

static jmp_buf saved_stack;
static void do_unwind(void *ctx){
    (void)ctx;
    longjmp(saved_stack,1);
}
struct rig {
    unsigned calls;
    uint32_t channel;
    uint8_t change_second,flags,ods1,bad_label,request_rebuild,ready,fail_rebuild;
    uint8_t did_rebuild,quota,message,ran_ods1,ran_ods2;
};
static void check(int ok,const char *message){
    if(!ok){fprintf(stderr,"FAIL: %s\n",message);exit(1);}
}
static uint32_t getchn(void *ctx,uint32_t channel,
                       struct port_sysmount_device *p,
                       struct port_sysmount_device *s){
    struct rig *r=ctx;
    check(++r->calls==1,"GETCHN must be first");r->channel=channel;
    memset(p,0,sizeof(*p));
    memcpy(p->name,"IDE",4);
    p->flags=r->flags;
    p->unit=23;
    p->physical_blocks=8064;
    p->sector_bytes=512;
    *s=*p;
    if(r->change_second)s->physical_blocks++;
    return 1;
}
static uint32_t read_home(void *ctx,uint32_t channel,const char *label,struct port_sysmount_state *state){
    struct rig *r=ctx;
    check(++r->calls==2,"READ_HOMEBLOCK must follow GETCHN");
    check(channel==r->channel && strcmp(label,"SYSTEMDISK")==0,"home label");
    state->level2=!r->ods1;
    return r->bad_label?18u:1u;
}
static uint32_t mounted(void *ctx,struct port_sysmount_state *s){
    struct rig *r=ctx;
    check(++r->calls==3,"MOUNT_DISK must follow home check");
    if(r->ods1)r->ran_ods1=1;else r->ran_ods2=1;
    check(strcmp(s->physical_name,"_IDE23:")==0,"physical unit formatting");
    check((s->mount_options & ((uint64_t)1u<<PORT_MOUNT_OPT_MOUNTVER))!=0u,"mount verify");
    s->volume_mounted=1;
    s->acp_started=r->ready;
    if(r->request_rebuild)s->cleanup_flags|=(1u<<PORT_MOUNT_CLF_REBUILD)|
                                  (1u<<PORT_MOUNT_CLF_REBUILDQUO);
    return 1u;
}
static uint32_t rebuild(void *ctx,uint32_t channel,uint8_t quota){
    struct rig *r=ctx;
    check(++r->calls==4,"rebuild after mount");
    check(channel==r->channel,"rebuild channel");
    r->did_rebuild=1;r->quota=quota;
    return r->fail_rebuild?22u:1u;
}
static void message(void *ctx){((struct rig*)ctx)->message++;}
static const struct port_sysmount_status statuses={12u,14u,16u,18u,28u,1u};
static void run(struct rig *r,struct port_sysmount_state *s,uint32_t expected){
    struct port_sysmount_services ops={getchn,read_home,mounted,mounted,
                                      rebuild,message,0,r};
    check(port_sysmount_system(s,&ops,&statuses,9u)==expected,"mount status");
}
int main(void){
    struct port_sysmount_state state;
    struct rig r={.flags=PORT_SYSMOUNT_FOD|PORT_SYSMOUNT_AVL,.ready=1};
    run(&r,&state,1);
    check(r.ran_ods2 && !r.ran_ods1,"use ODS2");
    check((state.mount_options&((uint64_t)1u<<PORT_MOUNT_OPT_IS_FILES11B))!=0u,"set ODS2 bit");
    check((state.mount_options&((uint64_t)1u<<PORT_MOUNT_OPT_IS_FILES11))!=0u,"set Files11 bit");
    check(state.device_count==1&&state.label_count==1,"parser defaults");
    r=(struct rig){.flags=PORT_SYSMOUNT_FOD|PORT_SYSMOUNT_AVL,.ready=1,
                   .bad_label=1,.ods1=1,.request_rebuild=1};
    run(&r,&state,1);
    check(r.ran_ods1&&!r.ran_ods2&&r.did_rebuild&&r.quota&&r.message==1,
          "ODS1/rebuild path");
    check(!(state.mount_options&((uint64_t)1u<<PORT_MOUNT_OPT_IS_FILES11B)),"ODS1 bit");
    r=(struct rig){.flags=PORT_SYSMOUNT_FOD|PORT_SYSMOUNT_AVL,.ready=1,
                   .change_second=1};run(&r,&state,12);
    check(r.calls==1,"reject mismatched DIB before home block");
    r=(struct rig){.flags=PORT_SYSMOUNT_AVL,.ready=1};run(&r,&state,12);
    r=(struct rig){.flags=PORT_SYSMOUNT_FOD,.ready=1};run(&r,&state,14);
    r=(struct rig){.flags=PORT_SYSMOUNT_FOD|PORT_SYSMOUNT_AVL|PORT_SYSMOUNT_MNT,
                   .ready=1};run(&r,&state,16);
    r=(struct rig){.flags=PORT_SYSMOUNT_FOD|PORT_SYSMOUNT_AVL|PORT_SYSMOUNT_SQD,
                   .ready=1};run(&r,&state,12);
    r=(struct rig){.flags=PORT_SYSMOUNT_FOD|PORT_SYSMOUNT_AVL,.ready=0};
    run(&r,&state,PORT_SYSMOUNT_INCOMPLETE);
    check(r.calls==3,"cannot claim mount before ACP started");
    r=(struct rig){.flags=PORT_SYSMOUNT_FOD|PORT_SYSMOUNT_AVL,.ready=1,
                   .request_rebuild=1,.fail_rebuild=1};run(&r,&state,22);
    {
        struct port_sysmount_services ops={getchn,read_home,mounted,mounted,
                                           rebuild,message,0,&r};
        uint32_t saved=0;
        check(port_sysmount_main_handler(&state,&ops,&statuses,1,&saved)==1,
              "continue normal");
        check(port_sysmount_main_handler(&state,&ops,&statuses,28,&saved)==1,
              "do not unwind unwind");
        check(port_sysmount_main_handler(&state,&ops,&statuses,44,&saved)==
              PORT_SYSMOUNT_NO_SERVICE,"severe requires unwind");
        check(saved==44,"severe returns original signal");
        ops.unwind=do_unwind;
        saved=0;
        if(setjmp(saved_stack)==0){
            (void)port_sysmount_main_handler(&state,&ops,&statuses,44,&saved);
            check(0,"severe $UNWIND must not return");
        }
        check(saved==44,"unwind preserves the severe status");
    }
    puts("PASS: SYSMOU system-mount ordering, device status, ODS1/2, ACP, rebuild and condition handler");
    return 0;
}

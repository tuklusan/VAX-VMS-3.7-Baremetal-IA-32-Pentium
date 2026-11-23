#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_exe_io.h"
#include "vms_exe_cancel.h"
#include "vms_exe_dassgn.h"
#include "vms_exe_devalc.h"
struct rig {
    uint8_t mem[512];
    uint32_t calls,posts,driver,deaccess,wait,volume,mailbox;
    struct port_exe_device *device;
};
static int read_user(void *p,uint32_t a,void *out,uint32_t n){struct rig *r=p;if(a>512||n>512-a)return 0;memcpy(out,r->mem+a,n);return 1;}
static int write_user(void *p,uint32_t a,const void *in,uint32_t n){struct rig *r=p;if(a>512||n>512-a)return 0;memcpy(r->mem+a,in,n);return 1;}
static int probe(void *p,uint32_t a,uint32_t n){(void)p;return a<=512&&n<=512-a;}
static void lock_io(void *p){((struct rig *)p)->calls++;}
static void unlock_io(void *p){((struct rig *)p)->calls++;}
static uint32_t driver_cancel(void *p,struct port_exe_device *d,uint16_t ch,uint8_t why){struct rig *r=p;(void)ch;(void)why;assert(d);r->driver++;return 1;}
static uint32_t driver_release(void *p,struct port_exe_device *d){struct rig *r=p;assert(d);r->driver++;return 1;}
static uint32_t post_io(void *p,struct port_exe_io_request *q){struct rig *r=p;assert(q->status==PORT_SS_CANCEL);r->posts++;return 1;}
static uint32_t deaccess(void *p,struct port_exe_io_channel *ch){struct rig *r=p;r->deaccess++;ch->has_file=0;return 1;}
static uint32_t wait_io(void *p,struct port_exe_io_channel *ch){struct rig *r=p;r->wait++;ch->outstanding=0;return 1;}
static uint32_t volume(void *p,void *v){struct rig *r=p;assert(v==p);r->volume++;return 1;}
static uint32_t mailbox(void *p,struct port_exe_device *d){struct rig *r=p;assert(d);r->mailbox++;return 1;}
static uint32_t find(void *p,uint32_t name,uint8_t allocated,struct port_exe_device **d){struct rig *r=p;(void)name;(void)allocated;*d=r->device;return 1;}
static uint32_t allowed(void *p,struct port_exe_device *d,uint32_t uic){(void)p;(void)d;(void)uic;return 1;}
int main(void){
 struct rig r={0};
 struct port_exe_device device={.name="DQA0:",.owner_uic=0x10001};
 r.device=&device;
 struct port_exe_access mem={&r,read_user,write_user,probe,probe};
 struct port_exe_device *devices[1]={&device};
 struct port_exe_device_ops dv={.context=&r,.memory=&mem,.devices=devices,.device_count=1,.lock_io=lock_io,.unlock_io=unlock_io,.find=find,.check_read_access=allowed,.driver_cancel=driver_release};
 struct port_exe_dev_request req={.access_mode=2};
 assert(port_exe_alloc(&device,7,1,3,0,&req,&dv)==1);
 assert(device.owner_pid==7 && device.references==1 && device.alloc_mode==3);
 assert(port_exe_alloc(&device,7,1,3,0,&req,&dv)==PORT_SS_DEVALRALLOC);
 assert(port_exe_dalloc(7,3,0,3,&dv)==1);
 assert(device.owner_pid==0 && device.references==0 && !(device.flags&PORT_EXE_DEV_ALLOCATED));
 device.flags=PORT_EXE_DEV_MOUNTED;
 assert(port_exe_alloc(&device,7,1,3,0,&req,&dv)==PORT_SS_DEVMOUNT);
 device.flags=PORT_EXE_DEV_SHAREABLE;
 assert(port_exe_alloc(&device,7,1,3,0,&req,&dv)==1);
 device.references=2;
 assert(port_exe_dalloc(7,3,0,3,&dv)==1 && device.references==2);
 struct port_exe_io_request q={.pid=7,.channel=5,.buffered=1,.queued=1};
 struct port_exe_io_channel ch={.owner_pid=7,.channel=5,.outstanding=1,.assigned=1,.access_mode=3,.has_file=1,.device=&device,.requests=&q};
 struct port_exe_io_ops io={.context=&r,.driver_cancel=driver_cancel,.post_io=post_io,.deaccess=deaccess,.wait_io=wait_io,.release_volume=volume,.delete_mailbox=mailbox,.lock=lock_io,.unlock=unlock_io};
 assert(port_exe_cancel(&ch,7,&io)==1);
 assert(r.posts==1 && r.driver==1+1 && q.status==PORT_SS_CANCEL && !q.queued);
 assert(port_exe_dassgn(&ch,7,&io)==1);
 assert(!ch.assigned && r.deaccess==1 && r.wait==1);
 assert(!device.references && !device.owner_pid);
 device.flags=PORT_EXE_DEV_SHAREABLE|PORT_EXE_DEV_TERMINAL|PORT_EXE_DEV_OPERATOR;
 device.owner_pid=7;device.references=1;device.alloc_mode=3;
 device.flags|=PORT_EXE_DEV_ALLOCATED;
 assert(port_exe_dalloc(7,3,32,3,&dv)==1);
 assert(!(device.flags&PORT_EXE_DEV_OPERATOR));
 puts("PASS: SYSDEVALC, SYSCANCEL and SYSDASSGN service paths");
}

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_sysinit_names.h"

struct fixture { unsigned created; const char *device; unsigned fail_at; };
static uint32_t resolve(void *ctx,const char *name,char *out,
                        unsigned cap,unsigned *received) {
    struct fixture *f=ctx;
    if(strcmp(name,"SYS$SYSDEVICE"))return 0;
    unsigned n=(unsigned)strlen(f->device);
    if(n>cap)return 0;
    memcpy(out,f->device,n);
    *received=n;
    return 1;
}
static uint32_t create(void *ctx,const char *name,const char *value) {
    struct fixture *f=ctx;
    static const char *const expected_names[]={"SYS$SYSTEM","SYS$SHARE","SYS$MESSAGE"};
    static const char *const expected_paths[]={"DUA0:[SYS0.SYSEXE]","DUA0:[SYS0.SYSLIB]","DUA0:[SYS0.SYSMSG]"};
    unsigned i=f->created;
    assert(i<3u);
    assert(!strcmp(name,expected_names[i]));
    assert(!strcmp(value,expected_paths[i]));
    f->created++;
    return f->fail_at==f->created?0u:1u;
}
int main(void) {
    const struct port_sip_logical_ops ops={resolve,create};
    const uint8_t top[10]={4,'S','Y','S','0'};
    struct fixture f={0,"_DUA0:",0};
    assert(port_sip_create_system_names(&ops,&f,top)==PORT_SIP_OK);
    assert(f.created==3u);
    f.created=0; f.fail_at=2;
    assert(port_sip_create_system_names(&ops,&f,top)==0u);
    assert(f.created==2u);
    f.fail_at=0;
    const uint8_t bad[10]={10,'T','O','O'};
    assert(port_sip_create_system_names(&ops,&f,bad)==PORT_SIP_BADARG);
    f.device="";
    assert(port_sip_create_system_names(&ops,&f,top)==PORT_SIP_TOOLONG);
    assert(port_sip_create_system_names(NULL,&f,top)==PORT_SIP_BADARG);
    puts("PASS: SYSINIT SYS$SYSTEM, SHARE, MESSAGE device and TOPSYS names");
    return 0;
}

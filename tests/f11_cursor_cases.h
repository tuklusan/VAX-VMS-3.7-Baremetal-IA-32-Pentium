#ifndef VMS37_F11_CURSOR_CASES_H
#define VMS37_F11_CURSOR_CASES_H
#include "vms_f11_cursor.h"
#include <stdio.h>
#include <string.h>
#define T(x) do { if(!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);return 1;} } while(0)
struct cursor_fixture {
 struct port_f11_chain chain;
 struct port_f11_boot_volume volume;
 struct port_f11_cursor cursor;
 uint32_t requests,fail_at, last_lbn;
};
static inline uint8_t disk_byte(uint32_t lbn,uint32_t index){return (uint8_t)(lbn*7u+index);}
static inline uint32_t cursor_disk(void *ctx,uint32_t lbn,uint8_t block[512]){
 struct cursor_fixture *f=(struct cursor_fixture *)ctx;
 f->last_lbn=lbn;
 ++f->requests;
 if((f->fail_at && f->requests==f->fail_at)||
    (!(lbn>=40u&&lbn<43u)&&!(lbn>=80u&&lbn<82u)))return PORT_DQ_DIR_BAD;
 for(uint32_t i=0;i<512u;i++)block[i]=disk_byte(lbn,i);
 return PORT_DQ_DIR_OK;
}
static inline void cursor_fixture_init(struct cursor_fixture *f){
 memset(f,0,sizeof *f);
 f->chain.count=2u;f->chain.blocks=5u;
 f->chain.extents[0]=(struct port_f11_extent){3u,40u};
 f->chain.extents[1]=(struct port_f11_extent){2u,80u};
 f->volume.read_sector=cursor_disk;f->volume.context=f;
 (void)port_f11_cursor_init(&f->cursor,&f->volume,&f->chain,0u);
}
#endif

#include "vms_f11_acp86.h"
#include "vms_logintime.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

static void put16(uint8_t *p,unsigned v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void put32(uint8_t *p,uint32_t v){put16(p,v);put16(p+2,v>>16);}
static uint32_t access_rights(void *ctx,uint32_t owner,uint16_t prot){
    unsigned *n=(unsigned *)ctx;
    assert(owner==0x10002u&&prot==0xabcd);
    ++*n;
    return 0x8421u;
}
struct lockmock {unsigned held,unlocks,ipl;};
static uint32_t lockw(void *p){struct lockmock *m=p;assert(!m->held);m->held=1;return 1;}
static uint32_t unlock(void *p){struct lockmock *m=p;assert(m->held);m->held=0;m->unlocks++;return 1;}
static void ipl0(void *p){struct lockmock *m=p;assert(!m->held);m->ipl++;}
struct iomock {unsigned physical,insert,ucb_seen;};
static uint32_t physical(void *ctx,void *u,uint32_t lbn,uint32_t *p,uint16_t *func){
    struct iomock *m=ctx;m->physical++;
    m->ucb_seen=(unsigned)(uintptr_t)u;
    *p=lbn+300u;*func=72u;return 1;
}
static uint32_t insert(void *ctx,struct port_f11_86_request *r){
    struct iomock *m=ctx;m->insert++;assert(r->func==72u);return 1;
}
struct mapmock {unsigned calls;uint32_t mapped,vbn,bytes;void *ucb;int success;};
static uint32_t map_vblk(void *ctx,void *w,uint32_t vbn,uint32_t bytes,
                         void **u,uint32_t *lbn,uint32_t *remaining){
    struct mapmock *m=ctx;assert(w==(void *)0x5u);
    m->calls++;m->vbn=vbn;m->bytes=bytes;
    *u=m->ucb;*lbn=122u;*remaining=m->mapped;
    return m->success?1u:12u;
}
struct hdrmock {unsigned calls;struct port_f11_86_fid last;unsigned segment;int ods1;};
static uint32_t read_header(void *ctx,struct port_f11_86_fid fid,void *fcb,uint8_t h[512]){
    struct hdrmock *m=ctx;
    assert(fcb==(void *)0x123u);
    m->last=fid;m->calls++;
    memset(h,0,512);
    h[1]=20;
    if(m->ods1)h[40]=(uint8_t)m->segment;
    else put16(h+4,m->segment);
    return 1;
}
static uint32_t now(void *p,uint64_t *t){*(unsigned *)p+=1u;return port_time_calendar_to_ticks(1982,4,26,16,21,0,t)?1u:12u;}

int main(int argc,char **argv){
    assert(argc==2);
    uint8_t block[512]={0};
    assert(port_f11a_checksum(block)==1u);
    block[2]=1u;
    assert(port_f11b_checksum(block)==0u&&block[510]==1u);
    assert(port_f11b_checksum(block)==1u);
    uint8_t name[]="DISK",out[32];uint16_t length=0;
    memset(out,0xaa,sizeof out);
    struct port_f11_86_abd d={name,4u,out,8u,&length};
    assert(port_f11a_copy_name(&d)==1u&&length==4u);
    assert(memcmp(out,"DISK\0\0\0\0",8)==0);
    d.result_count=2u;
    assert(port_f11b_copy_name(&d)==1u&&out[0]=='D'&&out[1]=='I'&&length==4u);
    d.result_count=15u;
    assert(port_f11a_return_dir((const uint8_t *)"SYSTEM.DIR;1",12u,&d)==1u);
    assert(length==12u&&memcmp(out,"SYSTEM.DIR;1",12)==0&&out[12]==0u);
    assert(port_f11b_return_dir((const uint8_t *)"SYS.EXE",7,17u,&d)==1u);
    assert(length==10u&&memcmp(out,"SYS.EXE;17",10)==0&&out[10]==0);
    assert(port_f11b_return_dir((const uint8_t *)"SYS.EXE",7,65535u,&d)==1u);
    assert(length==10u&&memcmp(out,"SYS.EXE;-1",10)==0);
    uint8_t maps[32]={0};
    put16(maps,0x4002u);put16(maps+2,30u);
    put16(maps+4,0x8003u);put32(maps+6,70000u);
    put16(maps+10,0xc000u);put16(maps+12,0x0010u);put32(maps+14,123456u);
    struct port_f11_86_map m={maps,18u,0,0,0};
    assert(port_f11b_get_map_pointer(&m)==1u&&m.blocks==3u&&m.lbn==30u&&m.offset==4u);
    assert(port_f11b_get_map_pointer(&m)==1u&&m.blocks==4u&&m.lbn==70000u&&m.offset==10u);
    assert(port_f11b_get_map_pointer(&m)==1u&&m.blocks==17u&&m.lbn==123456u&&m.offset==18u);
    assert(port_f11b_get_map_pointer(&m)==0u);
    uint8_t with_placement[6]={0};
    put16(with_placement,1u);put16(with_placement+2,0x4001u);put16(with_placement+4,14u);
    m=(struct port_f11_86_map){with_placement,6u,0u,0u,0u};
    assert(port_f11b_get_map_pointer(&m)==1u&&m.blocks==2u&&m.lbn==14u&&m.offset==6u);
    memset(block,0,512);block[1]=40u;block[7]=2u;block[58]=9u;
    memcpy(block+80,maps,18);
    uint32_t count=0;
    assert(port_f11b_file_size(block,&count)==1u&&count==24u);
    block[58]=240u;
    assert(port_f11b_file_size(block,&count)==0u);
    uint16_t rvn=0u;port_f11b_apply_rvn(&rvn,4u,0);assert(rvn==4u);
    port_f11b_default_rvn(&rvn,4u,0);assert(rvn==0u);
    rvn=0xa500u;port_f11b_apply_rvn(&rvn,3u,1);assert(rvn==0xa503u);
    port_f11b_default_rvn(&rvn,3u,1);assert(rvn==0xa500u);
    rvn=1u;port_f11b_apply_rvn(&rvn,0u,0);assert(rvn==0u);
    assert(port_f11b_match_name((const uint8_t *)"SYS.EXE",7,(const uint8_t *)"S*S.%XE",7));
    assert(port_f11b_match_name((const uint8_t *)"SYS.EXE",7,(const uint8_t *)"S*E",3));
    assert(port_f11b_match_name((const uint8_t *)"",0,(const uint8_t *)"*",1));
    assert(!port_f11b_match_name((const uint8_t *)"SYS.EXE",7,(const uint8_t *)"S*T",3));
    assert(!port_f11b_match_name((const uint8_t *)"SYS.EXE",7,(const uint8_t *)"SYS.%XE%%",9));
    port_f11a_scan_badlog();
    unsigned a=0;
    assert(port_f11a_get_access(&a,access_rights,0x10002u,0xabcdu)==15u);
    assert(port_f11b_get_access(&a,access_rights,0x10002u,0xabcdu)==15u&&a==2u);
    struct lockmock lc={0};
    const struct port_f11_86_lockops lo={lockw,unlock,ipl0,&lc};
    assert(port_f11a_lock_iodb(&lo)==1u&&lc.held);
    assert(port_f11a_unlock_iodb(&lo)==1u&&lc.ipl==1u);
    assert(port_f11b_lock_iodb(&lo)==1u&&lc.held);
    assert(port_f11b_unlock_iodb(&lo)==1u&&lc.ipl==2u&&lc.unlocks==2u);
    struct iomock im={0};
    const struct port_f11_86_iops io={physical,insert,&im};
    struct port_f11_86_request req={1600u,33u,(void *)0x9u,0u};
    assert(port_f11a_requeue(&req,(void *)0xau,22u,1u,&io)==1u);
    assert(req.bytes==1536u&&req.ucb==(void *)0x9u&&req.lbn==322u&&im.ucb_seen==10u);
    req.bytes=1600u;req.ucb=(void *)0x9u;
    assert(port_f11b_requeue(&req,(void *)0xau,22u,1u,&io)==1u);
    assert(req.bytes==1536u&&req.ucb==(void *)0xau&&im.insert==2u);
    struct mapmock mm={0};mm.mapped=512u;mm.ucb=(void *)0x7u;mm.success=1;
    const struct port_f11_86_mapops mo={map_vblk,&mm};
    uint32_t left=0;void *newucb=0;
    assert(port_f11a_map_window((void *)0x5u,(void *)0x9u,2u,3u,&left,&mo)==122u);
    assert(left==1u&&mm.bytes==1536u&&mm.vbn==2u);
    assert(port_f11b_map_window((void *)0x5u,(void *)0x9u,2u,3u,&left,&newucb,&mo)==122u);
    assert(newucb==(void *)0x7u&&left==1u);
    mm.success=0;
    assert(port_f11b_map_window((void *)0x5u,(void *)0x9u,2u,3u,&left,&newucb,&mo)==-1);
    assert(left==3u&&newucb==(void *)0x7u);
    struct hdrmock hm={0};hm.segment=3u;
    struct port_f11_86_file_control fc={(void *)0x123u};
    const struct port_f11_86_headerops ho={read_header,&hm};
    uint8_t old[512]={0},result[512]={0};int found=-1;
    old[1]=20u;put16(old+42,12u);put16(old+44,5u);old[40]=2u;hm.ods1=1;
    assert(port_f11a_next_header(old,&fc,0,0,&ho,result,&found)==1u);
    assert(found&&hm.last.number==12u&&hm.last.sequence==5u&&hm.calls==1u);
    hm.segment=5u;
    assert(port_f11a_next_header(old,&fc,0,0,&ho,result,&found)==PORT_F11_86_BADFILEHDR);
    hm.ods1=0;hm.segment=7u;
    put16(old+14,19u);put16(old+16,2u);put16(old+18,0u);put16(old+4,6u);
    assert(port_f11b_next_header(old,&fc,0,0,0,&ho,result,&found)==1u);
    assert(found&&hm.last.number==19u&&hm.last.sequence==2u&&hm.calls==3u);
    put16(old+14,27u);put16(old+16,4u);old[18]=2u;old[19]=0xa5u;
    hm.segment=7u;
    assert(port_f11b_next_header(old,&fc,0,0,1,&ho,result,&found)==1u);
    assert(found&&hm.last.number==27u&&hm.last.sequence==4u&&
           hm.last.volume==2u&&hm.last.number_extension==0xa5u);
    put16(old+14,0u);old[19]=0u;
    assert(port_f11b_next_header(old,&fc,0,0,1,&ho,result,&found)==1u&&!found);
    uint64_t ticks=0;
    assert(port_time_calendar_to_ticks(2024,2,29,6,7,8,&ticks));
    uint8_t b[13];
    assert(port_f11a_get_time(b,&ticks,0)==1u&&memcmp(b,"29FEB24060708",13)==0);
    assert(port_f11b_get_time(b,&ticks,0)==1u&&memcmp(b,"29FEB24060708",13)==0);
    ticks=0;
    assert(port_f11a_get_time(b,&ticks,0)==1u);
    for(unsigned i=0;i<13;i++)assert(b[i]==0u);
    assert(port_f11b_get_time(b,&ticks,0)==1u&&memcmp(b,"17NOV58000000",13)==0);
    unsigned now_calls=0;const struct port_f11_86_timeops ti={now,&now_calls};
    assert(port_f11b_get_time(b,0,&ti)==1u&&now_calls==1u&&memcmp(b,"26APR82162100",13)==0);
    FILE *f=fopen(argv[1],"rb");assert(f);
    assert(fseek(f,22L*512L,SEEK_SET)==0);
    assert(fread(block,1,512,f)==512u);fclose(f);
    assert(port_f11b_checksum(block)==1u);
    assert(port_f11b_file_size(block,&count)==1u&&count==3u);
    puts("F11A/F11B 23 source-file equivalents: host and real disk header PASS");
    return 0;
}

#include "vms_sys85.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static uint32_t finish(void *p,uint32_t s){*(uint32_t *)p=s;return s;}
static uint32_t allow(void *p,uint16_t v,uint32_t u){
    *(uint32_t *)p=u;return v==11?PORT_SYS85_NORMAL:PORT_SYS85_NOPRIV;
}
static uint32_t abort_request(void *p,uint32_t s){
    *(uint32_t *)p=s;return s;
}
static int fork_hits,irq_hits;
static char fork_order[16];
static void irq(void *p,uint8_t i){*(uint8_t *)p=i;irq_hits++;}
static void worker(struct port_sys85_fork *f,void *p){
    (void)p;fork_order[fork_hits++]=(char)f->saved_r3;
}
int main(void){
    assert(strcmp(port_nl_driver_dpt.name,"NLDRIVER")==0);
    assert(port_nl_driver_dpt.ucb_size==116 && port_nl_driver_dpt.count==5);
    uint32_t s=0;
    assert(port_nl_driver_io(PORT_SYS85_READLBLK,&s,finish)==PORT_SYS85_ENDOFFILE);
    assert(s==PORT_SYS85_ENDOFFILE);
    assert(port_nl_driver_io(PORT_SYS85_WRITEVBLK,&s,finish)==1&&s==1);
    assert(port_nl_driver_io(15,&s,finish)==PORT_SYS85_BADFUNC);
    struct port_sys85_rms r={0};
    uint32_t handles[8]={1,2,3,4,5,6,7,8};
    r.handle_table=handles;r.handle_table_count=8;r.active_ifabs=1;
    r.ifab_table_link=3;r.irab_table_link=4;r.segment_address=123;r.segment_length=512;
    assert(port_rm_reset(&r)==PORT_SYS85_MUSTCLOSEFL);
    assert(!r.segment_address&&!r.segment_length&&!r.active_ifabs);
    assert(!handles[7]&&!r.ifab_table_link&&!r.irab_table_link);
    assert(r.free_pages.next==&r.free_pages && r.free_space.prev==&r.free_space);
    assert(port_rm_reset(&r)==1);
    struct port_sys85_tape t={0,0x10,44,11};
    assert(port_mt_check_access(&t,&s,allow,abort_request)==1&&s==44);
    t.device_characteristics=0x10;
    assert(port_mt_check_access(&t,&s,allow,abort_request)==PORT_SYS85_WRITLCK);
    assert(s==PORT_SYS85_WRITLCK);
    t.device_characteristics=0;t.volume_protection=12;
    assert(port_mt_check_access(&t,&s,allow,abort_request)==PORT_SYS85_NOPRIV);
    struct port_sys85_fnb f={0};
    assert(port_fil_cvtfilnam((const uint8_t *)"SYSEXE.EXE;1",12,&f)==1);
    assert(f.filename[0]==(19*1600+25*40+19));
    assert(f.filename[1]==(5*1600+24*40+5));
    assert(f.filename[2]==0&&f.filetype==(5*1600+24*40+5));
    assert(f.version==1);
    assert(port_fil_cvtfilnam((const uint8_t *)"ABC.DEF;65535",13,&f)==1&&f.version==65535);
    assert(port_fil_cvtfilnam((const uint8_t *)"AB",2,&f)==1);
    assert(f.version==65535 && f.filetype==(4*1600+5*40+6));
    assert(port_fil_cvtfilnam((const uint8_t *)".TXT",4,&f)==1);
    assert(f.filename[0]==(1*1600+2*40));
    assert(f.filetype==(20*1600+24*40+20));
    assert(port_fil_cvtfilnam((const uint8_t *)"ABCDEFGHIJ.TXT",14,&f)==PORT_SYS85_BADFILENAME);
    assert(port_fil_cvtfilnam((const uint8_t *)"ABC.ABCD",8,&f)==PORT_SYS85_BADFILENAME);
    assert(port_fil_cvtfilnam((const uint8_t *)"ABC.DF;AB",9,&f)==PORT_SYS85_BADFILENAME);
    struct port_sys85_fork_system sys;
    uint8_t last_ipl=0;
    port_exe_fork_init(&sys,irq,&last_ipl);
    struct port_sys85_fork a={0},b={0};
    a.entry=worker;b.entry=worker;a.fork_ipl=b.fork_ipl=8;
    assert(port_exe_fork(&sys,&a,'A',3)==1);
    assert(port_exe_fork(&sys,&b,'B',4)==1);
    assert(last_ipl==8&&irq_hits==2);
    assert(port_exe_fork(&sys,&a,'Q',4)==PORT_SYS85_BADFUNC);
    assert(port_exe_fork_dispatch(&sys,8)==1);
    assert(fork_hits==2&&fork_order[0]=='A'&&fork_order[1]=='B');
    assert(!sys.head[8]&&!sys.tail[8]&&!sys.pending_levels);
    uint16_t device_status=0xFFFF;
    assert(port_exe_iofork(&sys,&a,'C',2,&device_status,0x1000)==1);
    assert((device_status&0x1000)==0);
    assert(port_exe_fork_dispatch(&sys,8)==1&&fork_order[2]=='C');
    puts("M85 executive device, RMS, filename, fork service tests PASS");
}

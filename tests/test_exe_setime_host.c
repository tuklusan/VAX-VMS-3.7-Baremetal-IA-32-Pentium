#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_exe_setime.h"
#include "vms_logintime.h"
struct rig {uint8_t mem[128];uint32_t todr,write_count,save_count,locked,unlocked;uint64_t saved_time;};
static int read_user(void *ctx,uint32_t addr,void *out,uint32_t n){struct rig *r=ctx;if(addr>128||n>128-addr)return 0;memcpy(out,r->mem+addr,n);return 1;}
static int write_user(void *ctx,uint32_t addr,const void *in,uint32_t n){struct rig *r=ctx;if(addr>128||n>128-addr)return 0;memcpy(r->mem+addr,in,n);return 1;}
static int probe(void *ctx,uint32_t addr,uint32_t n){(void)ctx;return addr<=128&&n<=128-addr;}
static uint32_t rtc_read(void *ctx,uint32_t *v){*v=((struct rig *)ctx)->todr;return 1;}
static uint32_t rtc_write(void *ctx,uint32_t v){struct rig *r=ctx;r->todr=v;r->write_count++;return 1;}
static uint32_t checked(void *ctx){(void)ctx;return 1;}
static uint32_t save(void *ctx,uint32_t todr,uint64_t now){struct rig *r=ctx;assert(r->todr==todr);r->saved_time=now;r->save_count++;return 1;}
static void locked(void *ctx){((struct rig *)ctx)->locked++;}
static void unlocked(void *ctx){((struct rig *)ctx)->unlocked++;}
int main(void){
 struct rig r={0};
 struct port_exe_access mem={&r,read_user,write_user,probe,probe};
 struct port_exe_setime_ops ops={&r,&mem,rtc_read,rtc_write,checked,save,locked,unlocked};
 struct port_exe_systime clock={0};
 uint64_t old,now;
 assert(port_time_calendar_to_ticks(1982,4,1,0,0,0,&old));
 assert(port_time_calendar_to_ticks(1982,5,2,3,4,5,&now));
 port_exe_systime_store(&clock,old);
 struct port_exe_timer_queue queue;
 port_exe_timer_init(&queue,old);
 queue.entries[0].kind=PORT_EXE_TQE_TMSNGL;
 queue.entries[0].absolute=0;
 queue.entries[0].when=old+UINT64_C(5000000);
 queue.entries[1].kind=PORT_EXE_TQE_TMSNGL;
 queue.entries[1].absolute=1;
 queue.entries[1].when=old+UINT64_C(7000000);
 queue.used=2;
 struct port_exe_time_state st={.clock=&clock,.timers=&queue};
 assert(write_user(&r,16,&now,8));
 assert(port_exe_setime(&st,16,0,1,&ops)==PORT_SS_NOPRIV);
 assert(port_exe_setime(&st,16,1,1,&ops)==PORT_SS_NORMAL);
 assert(r.save_count==1 && r.saved_time==now && r.locked==1 && r.unlocked==1);
 assert(queue.now==now && queue.entries[0].when==now+UINT64_C(5000000));
 assert(queue.entries[1].when==old+UINT64_C(7000000));
 assert(clock.low==(uint32_t)now && clock.high==(uint32_t)(now>>32));
 r.todr+=17;
 assert(port_exe_setime(&st,0,1,1,&ops)==PORT_SS_NORMAL);
 assert(queue.now==now+UINT64_C(1700000));
 assert(r.save_count==2);
 uint64_t bad=UINT64_MAX;
 assert(write_user(&r,16,&bad,8));
 assert(port_exe_setime(&st,16,1,1,&ops)==PORT_SS_IVTIME);
 assert(port_exe_setime(&st,125,1,1,&ops)==PORT_SS_ACCVIO);
 puts("PASS: SYSSETIME clock, TODR, delta timers and SYS.EXE write path");
}

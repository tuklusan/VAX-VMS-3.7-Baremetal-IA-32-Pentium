#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "vms_userptr.h"
static uint32_t pd[1024], pt[1024], pt_next[1024];
static uint8_t pa[4096], pb[4096], pc[4096];
static const uint32_t *table(void *unused,uint32_t base) {
    (void)unused;
    return base==0x5000u ? pt : base==0x6000u ? pt_next : 0;
}
static uint8_t *physical(void *unused,uint32_t base) {
    (void)unused;
    return base==0xa000u ? pa : base==0xb000u ? pb : base==0xc000u ? pc : 0;
}
int main(void) {
    struct port_user_map m={pd,table,0};
    pd[0]=0x5000u|7u;
    pt[1]=0xa000u|7u;
    pt[2]=0xb000u|7u;
    pt[3]=0xa000u|7u; /* virtual 0x3000 aliases 0x1000 */
    assert(port_user_outputs_disjoint(&m,0x1000,512,0x3000,8,physical,0)==-1);
    assert(port_user_outputs_disjoint(&m,0x1000,512,0x2000,8,physical,0)==0);
    /* Same frame but disjoint byte intervals must remain legal. */
    assert(port_user_outputs_disjoint(&m,0x1000,512,0x3200,8,physical,0)==0);
    assert(port_user_outputs_disjoint(&m,0x1000,513,0x3200,8,physical,0)==-1);
    pt[3]=0xc000u|7u;
    assert(port_user_outputs_disjoint(&m,0x1000,512,0x3000,8,physical,0)==0);
    pt[3]=0xc000u|5u;
    assert(port_user_outputs_disjoint(&m,0x1000,512,0x3000,8,physical,0)==-1);
    pt[3]=0xd000u|7u; /* not physically accessible */
    assert(port_user_outputs_disjoint(&m,0x1000,512,0x3000,8,physical,0)==-1);
    pt[3]=0xc000u|7u;
    /* Cross a 4MiB PDE boundary with independent physical backing. */
    pt[1023]=0xa000u|7u;
    pd[1]=0x6000u|7u;
    pt_next[0]=0xc000u|7u;
    assert(port_user_outputs_disjoint(&m,0x3ffff0,32,0x2000,8,physical,0)==0);
    pt_next[0]=0xa000u|7u; /* alias the start of the first output */
    assert(port_user_outputs_disjoint(&m,0x3ffff0,32,0x2000,8,physical,0)==0);
    assert(port_user_outputs_disjoint(&m,0x3ffff0,32,0x1000,8,physical,0)==-1);
    assert(port_user_outputs_disjoint(&m,0xfffffffeu,8,0x1000,8,physical,0)==-1);
    assert(port_user_outputs_disjoint(&m,0x1000,0,0x2000,8,physical,0)==-1);
    assert(port_user_outputs_disjoint(0,0x1000,8,0x2000,8,physical,0)==-1);
    puts("M38 pre-submit output alias and physical backing checks PASS");
}

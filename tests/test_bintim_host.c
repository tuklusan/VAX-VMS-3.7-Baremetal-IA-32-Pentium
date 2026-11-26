#include "vms_bintim.h"
#include "vms_logintime.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TICKS UINT64_C(10000000)

static uint64_t absolute(unsigned y,unsigned m,unsigned d,unsigned h,
                         unsigned min,unsigned s,unsigned hs) {
    uint64_t value=0;
    assert(port_time_calendar_to_ticks(y,m,d,h,min,s,&value));
    return value+(uint64_t)hs*100000u;
}

static void expect(const char *text,uint64_t now,uint64_t expected) {
    uint64_t actual=UINT64_C(0x123456789abcdef0);
    assert(port_vms_bintim(text,strlen(text),now,&actual)==
           PORT_BINTIM_NORMAL);
    if(actual!=expected){fprintf(stderr,"%s: got %llu expected %llu\n",text,
        (unsigned long long)actual,(unsigned long long)expected);assert(0);}
}

int main(void) {
    const uint64_t now=absolute(2024,2,29,12,34,56,78);
    expect("1 02:03:04.5",now,
           (uint64_t)(-(int64_t)(TICKS*(86400u+2u*3600u+3u*60u+4u)+5000000u)));
    expect("0",now,0u);
    expect("29-FEB-2024 12:34:56.785",now,
           absolute(2024,2,29,12,34,56,79));
    expect("29--2024",now,absolute(2024,2,29,12,34,56,78));
    expect("29-FEB-2024",now,absolute(2024,2,29,12,34,56,78));
    expect("  1-JAN-2000 0:0:0.999 ",now,
           absolute(2000,1,1,0,0,1,0));

    uint64_t unchanged=0x55u;
    assert(port_vms_bintim("31-FEB-2024",11u,now,&unchanged)==
           PORT_BINTIM_IVTIME);
    assert(unchanged==0x55u);
    assert(port_vms_bintim("1 24:00:00",10u,now,&unchanged)==
           PORT_BINTIM_IVTIME);
    assert(port_vms_bintim("1 00:00:00.x",12u,now,&unchanged)==
           PORT_BINTIM_IVTIME);
    assert(port_vms_bintim(NULL,1u,now,&unchanged)==PORT_BINTIM_IVTIME);
    puts("EXE$BINTIM absolute defaults, delta sign, range and rounding PASS");
    return 0;
}

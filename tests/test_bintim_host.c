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
    expect("31-DEC-2023 23:59:59.999",now,
           absolute(2024,1,1,0,0,0,0));

    uint64_t unchanged=0x55u;
    assert(port_vms_bintim("31-FEB-2024",11u,now,&unchanged)==
           PORT_BINTIM_IVTIME);
    assert(unchanged==0x55u);
    assert(port_vms_bintim("1 24:00:00",10u,now,&unchanged)==
           PORT_BINTIM_IVTIME);
    assert(port_vms_bintim("1 00:00:00.x",12u,now,&unchanged)==
           PORT_BINTIM_IVTIME);
    assert(port_vms_bintim(NULL,1u,now,&unchanged)==PORT_BINTIM_IVTIME);

    uint16_t numeric[7]={0};
    assert(port_vms_numtim(numeric,&now,0u)==PORT_NUMTIM_NORMAL);
    const uint16_t leap_expected[7]={2024u,2u,29u,12u,34u,56u,78u};
    assert(memcmp(numeric,leap_expected,sizeof numeric)==0);
    assert(port_vms_numtim(numeric,NULL,now)==PORT_NUMTIM_NORMAL);
    assert(memcmp(numeric,leap_expected,sizeof numeric)==0);
    const uint64_t delta=(uint64_t)(-(int64_t)(TICKS*(86400u+2u*3600u+
                                3u*60u+4u)+7800000u));
    assert(port_vms_numtim(numeric,&delta,now)==PORT_NUMTIM_NORMAL);
    const uint16_t delta_expected[7]={0u,0u,1u,2u,3u,4u,78u};
    assert(memcmp(numeric,delta_expected,sizeof numeric)==0);
    const uint64_t too_long=(uint64_t)(-(int64_t)(UINT64_C(10000)*86400u*TICKS));
    for(unsigned i=0u;i<7u;i++)numeric[i]=0xaaaa;
    assert(port_vms_numtim(numeric,&too_long,now)==PORT_NUMTIM_IVTIME);
    for(unsigned i=0u;i<7u;i++)assert(numeric[i]==0xaaaau);
    assert(port_vms_numtim(NULL,&now,0u)==PORT_NUMTIM_ACCVIO);
    const uint16_t epoch_expected[7]={1858u,11u,17u,0u,0u,0u,0u};
    const uint64_t epoch=0u;
    assert(port_vms_numtim(numeric,&epoch,now)==PORT_NUMTIM_NORMAL);
    assert(memcmp(numeric,epoch_expected,sizeof numeric)==0);
    const uint64_t century1900=absolute(1900,2,28,23,59,59,99);
    const uint16_t century1900_expected[7]={1900u,2u,28u,23u,59u,59u,99u};
    assert(port_vms_numtim(numeric,&century1900,now)==PORT_NUMTIM_NORMAL);
    assert(memcmp(numeric,century1900_expected,sizeof numeric)==0);
    const uint64_t century2000=absolute(2000,2,29,0,0,0,1);
    const uint16_t century2000_expected[7]={2000u,2u,29u,0u,0u,0u,1u};
    assert(port_vms_numtim(numeric,&century2000,now)==PORT_NUMTIM_NORMAL);
    assert(memcmp(numeric,century2000_expected,sizeof numeric)==0);
    const uint64_t century2100=absolute(2100,3,1,0,0,0,0);
    const uint16_t century2100_expected[7]={2100u,3u,1u,0u,0u,0u,0u};
    assert(port_vms_numtim(numeric,&century2100,now)==PORT_NUMTIM_NORMAL);
    assert(memcmp(numeric,century2100_expected,sizeof numeric)==0);
    puts("EXE$BINTIM and EXE$NUMTIM absolute/delta conversion and limits PASS");
    return 0;
}

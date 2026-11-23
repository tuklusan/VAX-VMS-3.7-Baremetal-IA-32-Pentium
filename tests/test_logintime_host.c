#include <stdint.h>
#include <stdio.h>
#include "vms_logintime.h"
#define CHECK(c) do { if (!(c)) {fprintf(stderr,"LOGINTIME test failure at %d\n",__LINE__);return 1;} } while(0)
struct stub { uint64_t ticks; unsigned calls; };
static int read_clock(void *context,uint64_t *ticks) {
    struct stub *stub=context;
    ++stub->calls;
    *ticks=stub->ticks;
    return 1;
}
static int check(unsigned y,unsigned m,unsigned d,unsigned h,unsigned min,unsigned s,
                 uint32_t weekday) {
    uint64_t t=0;
    uint32_t day=0,hour=0;
    CHECK(port_time_calendar_to_ticks(y,m,d,h,min,s,&t));
    CHECK(port_login_day_of_week(&t,&day,0)==PORT_LIB_NORMAL && day==weekday);
    CHECK(port_login_hour_of_day(&t,&hour,0)==PORT_LIB_NORMAL && hour==h);
    CHECK(day>=1 && day<=7);
    return 0;
}
int main(void) {
    uint64_t t=0;
    uint32_t day=0,hour=0;
    CHECK(check(1858,11,17,0,0,0,3)==0);
    CHECK(port_time_calendar_to_ticks(1858,11,17,0,0,0,&t) && t==0);
    CHECK(check(1858,11,18,23,59,59,4)==0);
    CHECK(check(1970,1,1,0,0,0,4)==0);
    CHECK(check(2024,2,29,12,31,45,4)==0);
    CHECK(check(2026,11,18,23,59,59,3)==0);
    CHECK(check(2030,1,1,0,0,0,2)==0);
    CHECK(!port_time_calendar_to_ticks(1900,2,29,0,0,0,&t));
    CHECK(!port_time_calendar_to_ticks(2026,13,1,0,0,0,&t));
    CHECK(!port_time_calendar_to_ticks(2026,12,1,24,0,0,&t));
    CHECK(!port_time_calendar_to_ticks(1858,11,16,0,0,0,&t));
    CHECK(port_login_day_of_week(0,&day,0)==PORT_LIB_INVARG);
    CHECK(port_login_hour_of_day(0,&hour,0)==PORT_LIB_INVARG);
    CHECK(port_login_day_of_week(&t,0,0)==PORT_LIB_INVARG);
    CHECK(port_login_hour_of_day(&t,0,0)==PORT_LIB_INVARG);
    struct stub stub={0,0};
    struct port_time_source source={read_clock,&stub};
    CHECK(port_login_day_of_week(0,&day,&source)==PORT_LIB_NORMAL && day==3);
    CHECK(port_login_hour_of_day(0,&hour,&source)==PORT_LIB_NORMAL && hour==0);
    CHECK(stub.calls==2);
    puts("PASS: LOGIN LIBDAYWK/LIBHOUR VMS epoch, leap years, live source, invalid arguments");
    return 0;
}

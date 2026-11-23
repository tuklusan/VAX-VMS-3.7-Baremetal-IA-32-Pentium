#include "vms_logintime.h"

/* VMS counts 100-ns intervals since 1858-11-17 at midnight. */
static uint64_t divide32(uint64_t value,uint32_t divisor) {
    uint64_t quotient=0;
    uint64_t rem=0;
    for (int bit=63; bit>=0; --bit) {
        rem=(rem<<1)|((value>>bit)&1u);
        if (rem>=divisor) {
            rem-=divisor;
            quotient |= UINT64_C(1)<<bit;
        }
    }
    return quotient;
}
static int get_time(const uint64_t *provided,const struct port_time_source *source,
                    uint64_t *ticks) {
    if (provided) { *ticks=*provided; return 1; }
    return source && source->read_ticks &&
           source->read_ticks(source->context,ticks);
}
int port_login_day_of_week(const uint64_t *vms_time,uint32_t *day,
                           const struct port_time_source *now) {
    uint64_t ticks;
    if (!day || !get_time(vms_time,now,&ticks)) return PORT_LIB_INVARG;
    uint64_t minutes=divide32(ticks,600000000u);
    uint64_t days=divide32(minutes,1440u);
    *day=(((uint32_t)days+2u)%7u)+1u;
    return PORT_LIB_NORMAL;
}
int port_login_hour_of_day(const uint64_t *vms_time,uint32_t *hour,
                           const struct port_time_source *now) {
    uint64_t ticks;
    if (!hour || !get_time(vms_time,now,&ticks)) return PORT_LIB_INVARG;
    uint64_t minutes=divide32(ticks,600000000u);
    uint64_t hours=divide32(minutes,60u);
    *hour=((uint32_t)hours%24u);
    return PORT_LIB_NORMAL;
}
static unsigned days_in_month(unsigned year,unsigned month) {
    static const uint8_t days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    if (month<1 || month>12) return 0;
    if (month==2 && year%4u==0 && (year%100u!=0 || year%400u==0))
        return 29;
    return days[month-1];
}
/* Gregorian days since 1970, with 1858-11-17 exactly 40587 days earlier. */
int port_time_calendar_to_ticks(unsigned year,unsigned month,unsigned day,
                                unsigned hour,unsigned minute,unsigned second,
                                uint64_t *ticks) {
    if (!ticks || year<1858 || year>9999 || !month || month>12 ||
        !day || day>days_in_month(year,month) ||
        hour>23 || minute>59 || second>59) return 0;
    int y=(int)year;
    y-=month<=2;
    int era=y/400;
    unsigned yoe=(unsigned)(y-era*400);
    unsigned m=(unsigned)((int)month + (month>2 ? -3 : 9));
    unsigned doy=(153u*m+2u)/5u+day-1u;
    unsigned doe=yoe*365u+yoe/4u-yoe/100u+doy;
    int64_t delta=(int64_t)(era*146097+(int)doe-719468+40587);
    if (delta<0) return 0;
    uint64_t seconds=(uint64_t)delta*86400u+(uint64_t)hour*3600u+
                     (uint64_t)minute*60u+second;
    *ticks=seconds*UINT64_C(10000000);
    return 1;
}

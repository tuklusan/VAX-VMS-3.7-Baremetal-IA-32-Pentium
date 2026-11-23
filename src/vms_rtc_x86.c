#include "vms_logintime.h"

/* Native PC CMOS time replaces a VAX GETTIM system call. */
static uint8_t rtc_read(uint8_t reg) {
    uint8_t out;
    __asm__ volatile("outb %b0,$0x70" :: "a"(reg));
    __asm__ volatile("inb $0x71,%b0" : "=a"(out));
    return out;
}
struct rtc_date {
    uint8_t sec,min,hour,day,month,year,century,mode;
};
static void rtc_snapshot(struct rtc_date *d) {
    d->mode=rtc_read(0x0b);
    d->sec=rtc_read(0x00);
    d->min=rtc_read(0x02);
    d->hour=rtc_read(0x04);
    d->day=rtc_read(0x07);
    d->month=rtc_read(0x08);
    d->year=rtc_read(0x09);
    d->century=rtc_read(0x32);
}
static int same_date(const struct rtc_date *a,const struct rtc_date *b) {
    return a->sec==b->sec && a->min==b->min && a->hour==b->hour &&
           a->day==b->day && a->month==b->month && a->year==b->year &&
           a->century==b->century && a->mode==b->mode;
}
static unsigned numeric(uint8_t value,int bcd) {
    return !bcd ? value : (value&15u)+10u*((unsigned)value>>4);
}
int port_rtc_x86_now(void *context,uint64_t *ticks) {
    (void)context;
    if (!ticks) return 0;
    struct rtc_date a,b;
    int ready=0;
    for (unsigned attempt=0;attempt<10;++attempt) {
        unsigned spins=0;
        while ((rtc_read(0x0a)&0x80u) && spins++<100000u) {}
        if (spins>=100000u) return 0;
        rtc_snapshot(&a);
        rtc_snapshot(&b);
        if (!(rtc_read(0x0a)&0x80u) && same_date(&a,&b)) {
            ready=1;
            break;
        }
    }
    if (!ready) return 0;
    int bcd=(a.mode&4u)==0;
    unsigned second=numeric(a.sec,bcd);
    unsigned minute=numeric(a.min,bcd);
    unsigned hour=numeric(a.hour&0x7fu,bcd);
    if (!(a.mode&2u)) hour=hour%12u+((a.hour&0x80u)?12u:0u);
    unsigned year=numeric(a.year,bcd);
    unsigned century=numeric(a.century,bcd);
    year+=(century>=19u && century<=29u ? century*100u :
           (year>=70u ? 1900u : 2000u));
    return port_time_calendar_to_ticks(year,numeric(a.month,bcd),
                                       numeric(a.day,bcd),hour,minute,second,ticks);
}

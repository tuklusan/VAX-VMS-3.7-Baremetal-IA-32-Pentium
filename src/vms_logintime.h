#ifndef VMS_LOGINTIME_H
#define VMS_LOGINTIME_H
#include <stdint.h>

/* LOGIN/SRC/LIBDAYWK.MAR and LOGIN/SRC/LIBHOUR.MAR. */
enum { PORT_LIB_NORMAL=1, PORT_LIB_INVARG=-2 };
struct port_time_source {
    int (*read_ticks)(void *context, uint64_t *ticks);
    void *context;
};
int port_login_day_of_week(const uint64_t *vms_time, uint32_t *day,
                           const struct port_time_source *now);
int port_login_hour_of_day(const uint64_t *vms_time, uint32_t *hour,
                           const struct port_time_source *now);
int port_time_calendar_to_ticks(unsigned year,unsigned month,unsigned day,
                                unsigned hour,unsigned minute,unsigned second,
                                uint64_t *ticks);
int port_rtc_x86_now(void *context,uint64_t *ticks);
#endif

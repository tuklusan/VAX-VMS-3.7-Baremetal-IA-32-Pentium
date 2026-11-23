#ifndef VMS37_DEVLOG_SEARCH_H
#define VMS37_DEVLOG_SEARCH_H
#include "vms_log_system.h"
#include "vms_ioc_phys.h"
/* IOSUBPAGD.MAR 302-531: source LOG$TRANSLATE loop followed by the
 * physical DDB/UCB search. SYSTEM bootstrap names only; no process/group
 * logicals, mutex, $ASSIGN, mounted volume, ACP or active DQ driver. */
enum { PORT_DEVLOG_MAX_STEPS=10u, PORT_DEVLOG_NAME_CAP=64u };
struct port_devlog_state {
    char name[PORT_DEVLOG_NAME_CAP];
    size_t length;
    uint8_t attempts,concealed,last_notran,stopped;
};
struct port_devlog_trace {
    char physical[PORT_DEVLOG_NAME_CAP];
    uint16_t length;
    uint8_t translations,concealed,last_notran,driver_bound;
    uint32_t ioc_status;
    struct port_v37_ioc_result device;
};
uint32_t port_devlog_input(struct port_devlog_state *,const char *,size_t);
uint32_t port_devlog_remote(const char *,size_t);
uint32_t port_devlog_colon(const char *,size_t,size_t *);
uint32_t port_devlog_underscore(const char *,size_t,uint8_t *);
uint32_t port_devlog_attempt(const struct port_log_system_table *,
    struct port_devlog_state *);
uint32_t port_devlog_normalize(const struct port_log_system_table *,
    struct port_devlog_state *);
uint32_t port_devlog_final(const struct port_devlog_state *,
    struct port_devlog_trace *);
uint32_t port_devlog_lookup(const uint8_t *,size_t,uint32_t,
    const struct port_v37_device_result *,unsigned,
    struct port_devlog_trace *);
uint32_t port_devlog_search(const struct port_log_system_table *,
    const uint8_t *,size_t,uint32_t,const struct port_v37_device_result *,
    const char *,size_t,unsigned,struct port_devlog_trace *);
#endif

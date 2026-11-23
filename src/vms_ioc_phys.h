#ifndef VMS37_NATIVE_IOC_PHYS_H
#define VMS37_NATIVE_IOC_PHYS_H
#include <stddef.h>
#include <stdint.h>
#include "vms_ini_devicedb.h"
/* SYS/SRC/IOSUBPAGD.MAR 260-531: source-form physical-device search.
 * LOG$TRANSLATE and IOC database locks are unported. NLA0 has one
 * software DDT; UNBOUND results and NLA0 matches do not grant channels. */
enum {
    PORT_V37_IOC_OK=1u,PORT_V37_IOC_BAD=2u,PORT_V37_IOC_UNBOUND=4u,
    PORT_V37_IOC_NO_SUCH=6u,PORT_V37_IOC_NONLOCAL=8u,
    PORT_V37_IOC_LOGICAL_PENDING=10u,PORT_V37_IOC_UNAVAILABLE=12u,
    PORT_V37_IOC_NAME_MAX=63u,PORT_V37_IOC_MODE_GENERIC=1u,
    PORT_V37_IOC_MODE_PHYSICAL=2u,PORT_V37_IOC_MODE_ALLOCATION=5u
};
struct port_v37_ioc_name {
    char controller[16];
    uint8_t length,concealed,explicit_unit,reserved;
    uint16_t unit;
};
struct port_v37_ioc_result {
    uint32_t ddb,ucb;
    uint16_t unit;
    uint8_t concealed,driver_bound;
};
/* All public helpers preserve their caller-owned destination on failure. */
uint32_t port_v37_ioc_upcase(const char *,size_t,char *,size_t);
uint32_t port_v37_ioc_strip(const char *,size_t,unsigned *,size_t *);
uint32_t port_v37_ioc_parse(const char *,size_t,struct port_v37_ioc_name *);
uint32_t port_v37_ioc_parse_unit(const char *,size_t,uint16_t *,uint8_t *);
uint32_t port_v37_ioc_validate(const uint8_t *,size_t,uint32_t,
              const struct port_v37_device_result *);
uint32_t port_v37_ioc_ddb(const uint8_t *,size_t,uint32_t,
              const struct port_v37_device_result *,const char *,size_t,
              uint32_t *,unsigned *);
uint32_t port_v37_ioc_ucb(const uint8_t *,size_t,uint32_t,
              const struct port_v37_device_result *,unsigned,uint16_t,
              unsigned,uint32_t *);
uint32_t port_v37_ioc_search(const uint8_t *,size_t,uint32_t,
              const struct port_v37_device_result *,const char *,size_t,
              unsigned,struct port_v37_ioc_result *);
uint32_t port_v37_ioc_searchdev(const uint8_t *,size_t,uint32_t,
              const struct port_v37_device_result *,const char *,size_t,
              struct port_v37_ioc_result *);
uint32_t port_v37_ioc_searchgen(const uint8_t *,size_t,uint32_t,
              const struct port_v37_device_result *,const char *,size_t,
              struct port_v37_ioc_result *);
uint32_t port_v37_ioc_searchalc(const uint8_t *,size_t,uint32_t,
              const struct port_v37_device_result *,const char *,size_t,
              struct port_v37_ioc_result *);
#endif

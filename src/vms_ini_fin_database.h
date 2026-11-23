#ifndef VMS_INI_FIN_DATABASE_H
#define VMS_INI_FIN_DATABASE_H
#include <stdint.h>
#include <stddef.h>
#include "vms_ini_devicedb.h"
#include "vms_ini_log_templates.h"
#include "vms_ini_syslog.h"
/* SYS/SRC/INIT.MAR FIN_DATABASE 1500-1559: build real counted device
 * equivalence strings from the selected boot DDB name and UCB unit.
 * This initializes storage only; LOG$TRANSLATE and system logical-name
 * lookup tables and full DQ driver activation remain unimplemented. */
enum {PORT_FINDB_OK=1u,PORT_FINDB_BAD=2u,PORT_FINDB_NO_ROOM=4u,
      PORT_FINDB_MISMATCH=6u,PORT_FINDB_DISK_EQ_OFF=29u,
      PORT_FINDB_SYSDEV_EQ_OFF=34u,PORT_FINDB_DISK_START=30u,
      PORT_FINDB_SYSDEV_START=35u};
struct port_ini_fin_settings {
    uint8_t *device_bytes;
    size_t device_size;
    uint32_t device_base;
    struct port_v37_device_result *device_info;
    uint8_t *disk_log,*sysdevice_log;
    size_t disk_log_size,sysdevice_log_size;
    uint16_t unit;
    uint8_t concealed;
    uint8_t adapter_is_ata;
};
struct port_ini_fin_result {
    uint8_t physical[12],system[13];
    uint8_t physical_count,system_count;
    uint16_t boot_unit;
    uint32_t boot_ddb,boot_ucb;
    uint8_t concealed,records_staged;
};
uint32_t port_ini_fin_decimal(uint16_t,char [5]);
uint32_t port_ini_fin_physical(uint16_t,char [12],uint8_t *);
uint32_t port_ini_fin_system(uint16_t,uint8_t,char [13],uint8_t *);
uint32_t port_ini_fin_validate(const struct port_ini_fin_settings *);
uint32_t port_ini_fin_ddb_name(uint8_t *,size_t,uint32_t,
    struct port_v37_device_result *);
void port_ini_fin_disk_record(uint8_t *,const char *,unsigned);
void port_ini_fin_sysdev_record(uint8_t *,const char *,unsigned);
uint32_t port_ini_fin_database(struct port_ini_fin_settings *,
    struct port_ini_fin_result *);
uint32_t port_ini_fin_check(const struct port_ini_fin_settings *,
    const struct port_ini_fin_result *);
#endif

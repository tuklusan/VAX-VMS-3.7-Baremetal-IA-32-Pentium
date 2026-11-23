#include "vms_log_system.h"
/* Do not publish a logical name unless both exact source-size records
 * were filled by FIN_DATABASE, without an active QIO or disk DDT. */
static int disjoint(const void *a,size_t x,const void *b,size_t y){
    const uintptr_t p=(uintptr_t)a,q=(uintptr_t)b;
    return x<=UINTPTR_MAX-p&&y<=UINTPTR_MAX-q&&
           (p+x<=q||q+y<=p);
}
static int exact(const uint8_t *r,size_t at,const char *s,unsigned n){
    if(r[at-1u]!=n)return 0;
    for(unsigned i=0u;i<n;i++)if(r[at+i]!=(uint8_t)s[i])return 0;
    return 1;
}
uint32_t port_log_system_validate(const struct port_log_system_table *t,
     uint8_t *disk,uint8_t *sysdev,size_t bytes){
    if(!t||!disk||!sysdev||bytes<PORT_LOG_REC_LEN||t->ready||t->count||
       !disjoint(t,sizeof *t,disk,PORT_LOG_REC_LEN)||
       !disjoint(t,sizeof *t,sysdev,PORT_LOG_REC_LEN)||
       !disjoint(disk,PORT_LOG_REC_LEN,sysdev,PORT_LOG_REC_LEN))return PORT_LOG_BAD;
    for(unsigned i=0u;i<PORT_LOG_SLOTS;i++)if(t->head[i])return PORT_LOG_BAD;
    if(port_log_record_valid(disk,bytes,"SYS$DISK",8u)!=PORT_LOG_OK||
       port_log_record_valid(sysdev,bytes,"SYS$SYSDEVICE",13u)!=PORT_LOG_OK||
       port_v37_read32(disk)!=0u||port_v37_read32(disk+4u)!=0u||
       port_v37_read32(sysdev)!=0u||port_v37_read32(sysdev+4u)!=0u||
       !exact(disk,30u,"_DQA0:",6u)||
       !exact(sysdev,35u,"__DQA0:",7u)||
       port_v37_read16(disk+12u)!=1u||port_v37_read16(sysdev+12u)!=1u||
       port_v37_read16(disk+14u)!=1u||port_v37_read16(sysdev+14u)!=1u||
       port_v37_read32(disk+16u)||port_v37_read32(sysdev+16u))return PORT_LOG_BAD;
    return PORT_LOG_OK;
}

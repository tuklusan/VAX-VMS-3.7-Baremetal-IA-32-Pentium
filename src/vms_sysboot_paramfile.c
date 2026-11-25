#include "vms_ini_sysparams.h"

static uint32_t read_le32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint8_t upper_ascii(uint8_t c){
    return c>='a'&&c<='z'?(uint8_t)(c-'a'+'A'):c;
}
struct param_field {
    const char *name;
    uint32_t *value;
    uint32_t minimum,maximum;
};

/* BOOTS/SRC/SYSBOOT.MAR BOO$USEFILE: 32 startup-file bytes, followed by
 * 16-byte names and 32-bit values, terminated by a zero longword. File
 * lookup and the full BOO$SEARCH descriptor table remain separate. */
uint32_t port_vms37_init_params_apply_file(
    struct port_vms37_init_params *params,const uint8_t *file,size_t bytes,
    uint8_t startup_file[32],uint32_t *applied_count){
    if(!params||!file||!startup_file||!applied_count||bytes<36u)
        return PORT_VMS37_SYSPARAM_FILE_BAD;
    struct port_vms37_init_params candidate=*params;
    uint8_t startup[32];
    for(unsigned i=0;i<sizeof startup;i++)startup[i]=file[i];
    uint32_t applied=0u;
    size_t offset=32u;
    for(;;){
        if(bytes-offset<4u)return PORT_VMS37_SYSPARAM_FILE_TRUNCATED;
        if(read_le32(file+offset)==0u){
            /* BOO$USEFILE tests only the longword at the current record
             * pointer.  READFILE's buffer tail is not part of the list and
             * may contain unrelated bytes. */
            *params=candidate;
            for(unsigned i=0;i<sizeof startup;i++)startup_file[i]=startup[i];
            *applied_count=applied;
            return PORT_VMS37_SYSPARAM_FILE_OK;
        }
        if(bytes-offset<20u)return PORT_VMS37_SYSPARAM_FILE_TRUNCATED;
        const uint8_t name_bytes=file[offset];
        if(!name_bytes||name_bytes>15u)return PORT_VMS37_SYSPARAM_FILE_BAD;
        const uint8_t *name=file+offset+1u;
        uint32_t value=read_le32(file+offset+16u);
        struct param_field fields[]={
            {"MAXPROCESSCNT",&candidate.max_processes,12u,8192u},
            {"REALTIME_SPTS",&candidate.realtime_spts,0u,UINT32_MAX},
            {"IRPCOUNT",&candidate.irp_count,0u,32768u},
            {"LRPCOUNT",&candidate.lrp_count,0u,4096u},
            {"LRPSIZE",&candidate.lrp_size,256u,16384u},
            {"SRPCOUNT",&candidate.srp_count,0u,4096u},
            {"SRPSIZE",&candidate.srp_size,96u,144u},
            {"NPAGEDYN",&candidate.nonpaged_dynamic_bytes,16384u,UINT32_MAX},
            {"MPW_WRTCLUSTER",&candidate.modified_page_fault_cluster,16u,120u},
            {"LOCKIDTBL",&candidate.lock_id_slots,16u,16000u},
            {"RESHASHTBL",&candidate.resource_hash_slots,1u,8192u},
            {"DEADLOCK_WAIT",&candidate.deadlock_wait_seconds,0u,UINT32_MAX},
            {"BALSETCNT",&candidate.balance_set_count,4u,1024u},
            {"WSMAX",&candidate.max_working_set,60u,16384u}
        };
        for(unsigned f=0;f<sizeof fields/sizeof fields[0];f++){
            size_t n=0;
            while(fields[f].name[n])n++;
            if(n!=name_bytes)continue;
            size_t j=0;
            while(j<n&&upper_ascii(name[j])==(uint8_t)fields[f].name[j])j++;
            if(j!=n)continue;
            if(value<fields[f].minimum)value=fields[f].minimum;
            if(value>fields[f].maximum)value=fields[f].maximum;
            *fields[f].value=value;
            applied++;
            break;
        }
        offset+=20u;
    }
}

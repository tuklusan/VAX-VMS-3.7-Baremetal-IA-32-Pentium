#include "vms_ini_sysparams.h"
#include <stdio.h>
#include <string.h>

static void put32(uint8_t *p,uint32_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);
    p[2]=(uint8_t)(x>>16);p[3]=(uint8_t)(x>>24);
}
static size_t add_param(uint8_t *p,const char *name,uint32_t value){
    size_t n=strlen(name);
    p[0]=(uint8_t)n;
    memcpy(p+1u,name,n);
    put32(p+16u,value);
    return 20u;
}

int main(void){
    struct port_vms37_init_params p={0};
    if(port_vms37_init_param_defaults(&p)!=PORT_VMS37_SYSPARAM_OK||
       p.max_processes!=72u||p.realtime_spts!=0u||p.irp_count!=60u||
       p.lrp_count!=4u||p.lrp_size!=576u||p.srp_count!=120u||
       p.srp_size!=96u||p.nonpaged_dynamic_bytes!=64000u||
       p.modified_page_fault_cluster!=96u||
       p.lock_id_slots!=128u||
       p.resource_hash_slots!=32u||p.deadlock_wait_seconds!=10u||
       p.balance_set_count!=36u||p.max_working_set!=1024u||
       port_vms37_init_param_defaults(0)!=PORT_VMS37_SYSPARAM_BAD){
        fputs("VMS 3.7 SYSPARAM defaults failed\n",stderr);
        return 1;
    }
    uint8_t file[32u+4u*20u+4u]={0},startup[32];
    memcpy(file,"SYS$SYSTEM:STARTUP.COM",22u);
    size_t off=32u;
    off+=add_param(file+off,"MAXPROCESSCNT",9000u);
    off+=add_param(file+off,"NPAGEDYN",1024u);
    off+=add_param(file+off,"LRPCOUNT",7u);
    off+=add_param(file+off,"NOT_A_PARAMETER",12u);
    uint32_t applied=0;
    if(port_vms37_init_params_apply_file(&p,file,off+4u,startup,
          &applied)!=PORT_VMS37_SYSPARAM_FILE_OK||
       applied!=3u||p.max_processes!=8192u||
       p.nonpaged_dynamic_bytes!=16384u||p.lrp_count!=7u||
       p.srp_count!=120u||memcmp(startup,file,32u)!=0){
        fputs("VMS SYSBOOT parameter-file merge failed\n",stderr);
        return 1;
    }
    struct port_vms37_init_params unchanged=p;
    if(port_vms37_init_params_apply_file(&p,file,off+3u,startup,
          &applied)!=PORT_VMS37_SYSPARAM_FILE_TRUNCATED||
       memcmp(&p,&unchanged,sizeof p)!=0){
        fputs("VMS SYSBOOT truncated parameter file was not atomic\n",stderr);
        return 1;
    }
    file[32u]=16u;
    if(port_vms37_init_params_apply_file(&p,file,off+4u,startup,
          &applied)!=PORT_VMS37_SYSPARAM_FILE_BAD||
       memcmp(&p,&unchanged,sizeof p)!=0){
        fputs("VMS SYSBOOT malformed parameter name was not rejected\n",stderr);
        return 1;
    }
    puts("VMS 3.7 INIT SYSPARAM defaults match original parameter declarations");
    return 0;
}

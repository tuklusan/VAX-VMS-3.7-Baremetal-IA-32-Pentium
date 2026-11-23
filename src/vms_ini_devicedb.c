#include "vms_ini_devicedb.h"
/* DEVICEDAT UCB macro and DDB macro expanded to fixed IA-32 bytes.
 * All pointer values are actual absolute resident 32-bit addresses; neither
 * VAX instructions nor fictitious device handler addresses are emulated. */
void port_v37_write16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
void port_v37_write32(uint8_t *p,uint32_t v){
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
uint16_t port_v37_read16(const uint8_t *p){return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));}
uint32_t port_v37_read32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
void port_v37_ddb_base(uint8_t *d,uint32_t next,uint32_t first,
    const char *name,unsigned name_len,const char *driver,unsigned driver_len){
    port_v37_write32(d+0u,next);port_v37_write32(d+4u,first);
    port_v37_write16(d+8u,PORT_V37_DDB_SIZE);d[10]=PORT_V37_DYN_DDB;
    d[20]=(uint8_t)name_len;
    for(unsigned i=0;i<name_len;i++)d[21u+i]=(uint8_t)name[i];
    d[36]=(uint8_t)driver_len;
    for(unsigned i=0;i<driver_len;i++)d[37u+i]=(uint8_t)driver[i];
}
void port_v37_ucb_base(uint8_t *u,uint32_t address,uint32_t ddb,uint32_t bytes){
    port_v37_write16(u+8u,(uint16_t)bytes);
    u[10u]=PORT_V37_DYN_UCB;
    port_v37_write32(u+28u,0x00010001u); /* standard [1,1] ownership */
    port_v37_write32(u+36u,ddb);
    port_v37_write32(u+64u,address+64u); /* source UCB macro IOQFL/IOQBL */
    port_v37_write32(u+68u,address+64u);
}
static int range32(uint32_t base,size_t n){
    return base && !(base&3u) && n>=PORT_V37_DEVICE_BYTES &&
        (uint64_t)base+PORT_V37_DEVICE_BYTES<=UINT64_C(0x100000000);
}
uint32_t port_v37_devices_prelink(uint8_t *memory,size_t cap,uint32_t base,
                                  struct port_v37_device_result *out){
    if(!memory || !out || ((uintptr_t)memory&3u) ||
       !range32(base,cap))return PORT_V37_DEV_BAD;
    /* Transactional preflight: valid callers see zeroed source-style
     * records; invalid calls cannot modify the supplied byte span. */
    for(unsigned i=0;i<PORT_V37_DEVICE_BYTES;i++)memory[i]=0u;
    const uint32_t ddb[4]={base+PORT_V37_DDB_BOOT_OFF,base+PORT_V37_DDB_OPA_OFF,
                          base+PORT_V37_DDB_MB_OFF,base+PORT_V37_DDB_NL_OFF};
    const uint32_t ucb[5]={base+PORT_V37_UCB_BOOT_OFF,base+PORT_V37_UCB_OPA_OFF,
                          base+PORT_V37_UCB_MB1_OFF,base+PORT_V37_UCB_MB2_OFF,
                          base+PORT_V37_UCB_NL_OFF};
    port_v37_ddb_boot(memory+PORT_V37_DDB_BOOT_OFF,ddb[1],ucb[0]);
    port_v37_ddb_opa(memory+PORT_V37_DDB_OPA_OFF,ddb[2],ucb[1]);
    port_v37_ddb_mb(memory+PORT_V37_DDB_MB_OFF,ddb[3],ucb[2]);
    port_v37_ddb_nl(memory+PORT_V37_DDB_NL_OFF,ucb[4]);
    port_v37_ucb_boot(memory+PORT_V37_UCB_BOOT_OFF,ucb[0],ddb[0]);
    port_v37_ucb_opa(memory+PORT_V37_UCB_OPA_OFF,ucb[1],ddb[1]);
    port_v37_ucb_mb1(memory+PORT_V37_UCB_MB1_OFF,ucb[2],ddb[2],ucb[3]);
    port_v37_ucb_mb2(memory+PORT_V37_UCB_MB2_OFF,ucb[3],ddb[2]);
    port_v37_ucb_nl(memory+PORT_V37_UCB_NL_OFF,ucb[4],ddb[3]);
    struct port_v37_device_result candidate={0};
    candidate.list_head=ddb[0];
    for(unsigned i=0;i<4;i++)candidate.ddb[i]=ddb[i];
    for(unsigned i=0;i<5;i++)candidate.ucb[i]=ucb[i];
    candidate.bytes=PORT_V37_DEVICE_BYTES;
    candidate.incomplete_driver_mask=0x0fu; /* NO DDT/CRB binding allowed */
    if(port_v37_devices_check(memory,cap,base,&candidate)!=PORT_V37_DEV_OK)
        return PORT_V37_DEV_BAD;
    *out=candidate;
    return PORT_V37_DEV_OK;
}
uint32_t port_v37_devices_check(const uint8_t *m,size_t cap,uint32_t base,
                                 const struct port_v37_device_result *r){
    if(!m || !r || ((uintptr_t)m&3u) || !range32(base,cap) ||
       r->list_head!=base || r->bytes!=PORT_V37_DEVICE_BYTES ||
       (r->incomplete_driver_mask!=15u && r->incomplete_driver_mask!=7u) ||
       r->boot_name_ready>1u ||
       (r->incomplete_driver_mask==15u && r->bound_nl_ddt) ||
       (r->incomplete_driver_mask==7u &&
          (!r->bound_nl_ddt||(r->bound_nl_ddt&3u) ||
           ((uint64_t)r->bound_nl_ddt>=base &&
            (uint64_t)r->bound_nl_ddt<(uint64_t)base+PORT_V37_DEVICE_BYTES))) ||
       (r->bound_dqa_crb && ((r->bound_dqa_crb&3u) ||
          ((uint64_t)r->bound_dqa_crb>=base &&
           (uint64_t)r->bound_dqa_crb<(uint64_t)base+PORT_V37_DEVICE_BYTES))))
        return PORT_V37_DEV_BAD;
    const unsigned ddb_offset[4]={0u,52u,104u,156u};
    const unsigned ucb_offset[5]={208u,460u,780u,912u,1044u};
    const unsigned sz[5]={252u,320u,132u,132u,132u};
    const unsigned owner[5]={0x10001u,0x10001u,0x10004u,0x10004u,0x10001u};
    const unsigned backlink[5]={0u,1u,2u,2u,3u};
    const unsigned bufsize[5]={512u,132u,1024u,256u,132u};
    const char *names[4]={r->boot_name_ready?"DQA":"","OPA","MBA","NLA"};
    const char *drivers[4]={r->boot_name_ready?"DQDRIVER":"","OPERATOR","MBDRIVER","NLDRIVER"};
    const unsigned nlen[4]={r->boot_name_ready?3u:0u,3u,3u,3u};
    const unsigned dlen[4]={r->boot_name_ready?8u:0u,8u,8u,8u};
    for(unsigned i=0;i<4;i++){
        const uint8_t *d=m+ddb_offset[i];
        if(r->ddb[i]!=base+ddb_offset[i] ||
           port_v37_read32(d)!=(i<3u?r->ddb[i+1u]:0u) ||
           port_v37_read32(d+4u)!=r->ucb[i==3u?4u:i] ||
           port_v37_read16(d+8u)!=PORT_V37_DDB_SIZE ||
           d[10u]!=PORT_V37_DYN_DDB || d[11u]!=0u ||
           port_v37_read32(d+12u)!=(i==3u?r->bound_nl_ddt:0u) || d[20u]!=nlen[i] ||
           d[36u]!=dlen[i])return PORT_V37_DEV_BAD;
        for(unsigned j=0;j<nlen[i];j++)if(d[21u+j]!=(uint8_t)names[i][j])return PORT_V37_DEV_BAD;
        for(unsigned j=0;j<dlen[i];j++)if(d[37u+j]!=(uint8_t)drivers[i][j])return PORT_V37_DEV_BAD;
    }
    if(port_v37_read32(m+16u)!=((uint32_t)'F'|((uint32_t)'1'<<8)|
                                  ((uint32_t)'1'<<16)|(1u<<24)))return PORT_V37_DEV_BAD;
    for(unsigned i=0;i<5;i++){
        const uint8_t *u=m+ucb_offset[i];
        if(r->ucb[i]!=base+ucb_offset[i] ||
           port_v37_read16(u+8u)!=sz[i] || u[10u]!=PORT_V37_DYN_UCB ||
           port_v37_read32(u+28u)!=owner[i] ||
           port_v37_read32(u+36u)!=r->ddb[backlink[i]] ||
           port_v37_read32(u+32u)!=(i==0u?r->bound_dqa_crb:0u) ||
           port_v37_read32(u+64u)!=r->ucb[i]+64u ||
           port_v37_read32(u+68u)!=r->ucb[i]+64u ||
           port_v37_read16(u+58u)!=bufsize[i] ||
           port_v37_read32(u+120u)!=(i==4u?r->bound_nl_ddt:0u))return PORT_V37_DEV_BAD;
    }
    if(m[PORT_V37_UCB_BOOT_OFF+11u]!=8u ||
       m[PORT_V37_UCB_BOOT_OFF+82u]!=21u ||
       m[PORT_V37_UCB_OPA_OFF+11u]!=8u ||
       m[PORT_V37_UCB_OPA_OFF+82u]!=20u ||
       m[PORT_V37_UCB_NL_OFF+82u]!=8u ||
       port_v37_read16(m+PORT_V37_UCB_OPA_OFF+88u)!=PORT_V37_UCB_ONLINE ||
       port_v37_read16(m+PORT_V37_UCB_NL_OFF+88u)!=PORT_V37_UCB_ONLINE ||
       port_v37_read16(m+PORT_V37_UCB_MB1_OFF+88u)!=PORT_V37_UCB_ONLINE ||
       port_v37_read16(m+PORT_V37_UCB_MB2_OFF+88u)!=PORT_V37_UCB_ONLINE ||
       port_v37_read32(m+PORT_V37_UCB_MB1_OFF+44u)!=r->ucb[3] ||
       port_v37_read32(m+PORT_V37_UCB_MB2_OFF+44u)!=0u ||
       port_v37_read16(m+PORT_V37_UCB_MB1_OFF+20u)!=60u ||
       port_v37_read16(m+PORT_V37_UCB_MB2_OFF+20u)!=20u ||
       port_v37_read16(m+PORT_V37_UCB_OPA_OFF+240u)!=132u ||
       m[PORT_V37_UCB_BOOT_OFF+112u]!=8u ||
       m[PORT_V37_UCB_BOOT_OFF+113u]!=8u)return PORT_V37_DEV_BAD;
    return PORT_V37_DEV_OK;
}

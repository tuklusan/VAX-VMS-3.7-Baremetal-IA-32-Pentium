#include "vms_exe_getsyi.h"

static uint16_t read16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));
}
static uint32_t read32(const uint8_t *p) {
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void put16(uint8_t *p,uint16_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
}
static void put32(uint8_t *p,uint32_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24);
}
static int readable(const struct port_syi_ops *ops,uint32_t address,
                    void *target,uint32_t count) {
    if(count && address>UINT32_MAX-(count-1u))return 0;
    return ops->read(ops->context,address,target,count);
}
static int writable(const struct port_syi_ops *ops,uint32_t address,
                    const void *value,uint32_t count) {
    if(count && address>UINT32_MAX-(count-1u))return 0;
    return ops->write(ops->context,address,value,count);
}

uint32_t port_exe_getsyi(const struct port_syi_process *process,
                         const struct port_syi_system *system,
                         const struct port_syi_ops *ops,
                         const struct port_syi_request *request) {
    if(!process||!system||!ops||!request||!ops->read||!ops->write||
       !ops->probe_write||!ops->clear_event||!ops->post_event||
       (request->ast_address && !ops->declare_ast))return PORT_SS_BADPARAM;

    uint32_t status=PORT_SS_NORMAL;
    uint8_t zeros[8]={0};
    if(request->iosb && !writable(ops,request->iosb,zeros,8u)) {
        status=PORT_SS_ACCVIO;
        goto complete;
    }
    status=ops->clear_event(ops->context,request->efn&255u);
    if(!(status&1u))goto complete;
    if(request->ast_address && !process->astcnt) {
        status=PORT_SS_EXASTLM;
        goto complete;
    }
    if(!request->item_list) {
        status=PORT_SS_BADPARAM;
        goto complete;
    }
    uint32_t address=request->item_list;
    /* Every step validates the next descriptor before using its pointers,
     * as IFNORD in the original does. A corrupt unterminated list fails. */
    for(uint32_t entries=0;entries<4096u;entries++) {
        uint8_t header[4];
        if(!readable(ops,address,header,4u)) {
            status=PORT_SS_ACCVIO;
            goto complete;
        }
        uint16_t capacity=read16(header);
        uint16_t item=read16(header+2);
        if(!item) {
            status=PORT_SS_NORMAL;
            goto complete;
        }
        uint8_t rest[12];
        if(address>UINT32_MAX-4u||
           !readable(ops,address+4u,rest,12u)) {
            status=PORT_SS_ACCVIO;
            goto complete;
        }
        uint32_t output=read32(rest);
        uint32_t length_address=read32(rest+4);
        uint8_t kind=(uint8_t)item;
        uint8_t section=(uint8_t)(item>>8);
        /* Reserved item bits are not accepted as aliases. */
        if((section!=1u && section!=2u)||
           (section==1u && kind!=0u)||
           (section==2u && kind>1u)) {
            status=PORT_SS_BADPARAM;
            goto complete;
        }
        uint8_t bytes[8]={0};
        uint32_t actual=section==1u?8u:kind==0u?1u:4u;
        if(section==1u) {
            for(unsigned i=0;i<8u;i++)bytes[i]=system->version[i];
        } else if(kind==0u)bytes[0]=system->cpu_type;
        else put32(bytes,system->sid);

        /* MOVC5 fills extra destination bytes with zeros. Writes are
         * checked through the caller's native page table mappings. */
        uint32_t n=capacity;
        if(n && !ops->probe_write(ops->context,output,n)) {
            status=PORT_SS_ACCVIO;
            goto complete;
        }
        for(uint32_t base=0;base<n;) {
            uint8_t chunk[32]={0};
            uint32_t take=n-base;
            if(take>sizeof(chunk))take=sizeof(chunk);
            for(uint32_t k=0;k<take;k++)
                if(base+k<actual)chunk[k]=bytes[base+k];
            if(output>UINT32_MAX-base ||
               !writable(ops,output+base,chunk,take)) {
                status=PORT_SS_ACCVIO;
                goto complete;
            }
            base+=take;
        }
        if(length_address) {
            uint8_t result[2];
            put16(result,(uint16_t)(capacity<actual?capacity:actual));
            if(!writable(ops,length_address,result,2u)) {
                status=PORT_SS_ACCVIO;
                goto complete;
            }
        }
        if(address>UINT32_MAX-12u) {
            status=PORT_SS_ACCVIO;
            goto complete;
        }
        address+=12u;
    }
    status=PORT_SS_ACCVIO;

complete:
    /* The original posts the event and writes final status even on error.
     * The IOSB's second longword stays cleared after its initial check. */
    ops->post_event(ops->context,process->pid,request->efn&255u);
    if(request->iosb) {
        uint8_t v[4];
        put32(v,status);
        (void)writable(ops,request->iosb,v,4u);
    }
    if(request->ast_address)
        ops->declare_ast(ops->context,request->ast_address,
                         request->ast_parameter,process->previous_mode&3u);
    return status;
}
int port_syi_user_read(void *context,uint32_t address,void *target,uint32_t bytes) {
    const struct port_syi_user *u=(const struct port_syi_user *)context;
    return u && port_user_copy_from(u->map,target,address,bytes,
                                    u->physical,u->context)==PORT_USER_OK;
}
int port_syi_user_write(void *context,uint32_t address,const void *source,uint32_t bytes) {
    const struct port_syi_user *u=(const struct port_syi_user *)context;
    return u && port_user_copy_to(u->map,address,source,bytes,
                                  u->physical,u->context)==PORT_USER_OK;
}

int port_syi_user_probe(void *context,uint32_t address,uint32_t bytes) {
    const struct port_syi_user *u=(const struct port_syi_user *)context;
    if(!u||!u->map||!u->physical||
       (bytes && address>UINT32_MAX-(bytes-1u)))return 0;
    uint32_t at=address,left=bytes;
    while(left) {
        uint32_t take=4096u-(at&4095u);
        if(take>left)take=left;
        if(port_user_range_valid(u->map,at,take,1))return 0;
        const struct port_user_map *m=u->map;
        uint32_t dir=m->pde[(at>>22)&1023u];
        const uint32_t *pte=m->resolve(m->context,dir&0xfffff000u);
        if(!pte||!u->physical(u->context,pte[(at>>12)&1023u]&0xfffff000u))
            return 0;
        at+=take;left-=take;
    }
    return 1;
}

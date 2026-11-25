#include "vms_ioc_mapvblk.h"
#include "vms_v37_dyn_tags.h"

enum { WCB_SIZE=8u,WCB_TYPE=10u,WCB_ACCESS=11u,WCB_ORGUCB=16u,
       WCB_NMAP=22u,WCB_FCB=24u,WCB_RVT=28u,WCB_LINK=32u,WCB_STVBN=44u,
       FM2_BYTES=6u,MAX_WCB_SEGMENTS=256u };

static uint16_t rd16(const uint8_t *p){
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));
}
static uint32_t rd32(const uint8_t *p){
    return (uint32_t)rd16(p)|((uint32_t)rd16(p+2)<<16);
}
static void result_init(struct port_ioc_mapvblk_result *r,uint32_t bytes,
                        uint32_t ucb){
    *r=(struct port_ioc_mapvblk_result){0u,bytes,ucb};
}
static uint32_t map_segment(const uint8_t *wcb,size_t capacity,
    uint32_t vbn,uint32_t bytes,uint32_t ucb,
    const struct port_ioc_mapvblk_ops *ops,
    struct port_ioc_mapvblk_result *out){
    if(capacity<WCB_SIZE||rd16(wcb+WCB_SIZE)<PORT_IOC_MAPVBLK_WCB_BYTES||
       rd16(wcb+WCB_SIZE)>capacity||wcb[WCB_TYPE]!=PORT_V37_DYN_WCB)
        return PORT_IOC_MAPVBLK_BADWCB;
    const uint16_t nmap=rd16(wcb+WCB_NMAP);
    if((uint64_t)PORT_IOC_MAPVBLK_WCB_BYTES+(uint64_t)nmap*FM2_BYTES>capacity)
        return PORT_IOC_MAPVBLK_BADWCB;
    const uint32_t stvbn=rd32(wcb+WCB_STVBN);
    if(!stvbn)return PORT_IOC_MAPVBLK_CORRUPT;
    uint32_t relative=0u,available=0u,lbn=0u;
    const uint32_t file_va=rd32(wcb+WCB_FCB);
    const uint8_t access=wcb[WCB_ACCESS];
    if(ops->is_sequential(ops->context,ucb)){
        if(!nmap)return PORT_IOC_MAPVBLK_UNMAPPED;
        out->ucb=rd32(wcb+PORT_IOC_MAPVBLK_WCB_BYTES);
        out->lbn=0u;out->unmapped_bytes=0u;
        return PORT_IOC_MAPVBLK_OK;
    }
    if(vbn<stvbn)return PORT_IOC_MAPVBLK_UNMAPPED;
    relative=vbn-stvbn;
    for(uint32_t i=0u;i<nmap;i++){
        const uint8_t *entry=wcb+PORT_IOC_MAPVBLK_WCB_BYTES+i*FM2_BYTES;
        const uint32_t count=rd16(entry);
        const uint32_t start=rd32(entry+2u);
        if(!count||start>UINT32_MAX-count)return PORT_IOC_MAPVBLK_CORRUPT;
        if(relative<count){
            lbn=start+relative;
            available=count-relative;
            if(i+1u<nmap){
                const uint8_t *next=entry+FM2_BYTES;
                const uint32_t next_count=rd16(next);
                const uint32_t next_lbn=rd32(next+2u);
                if(!next_count||next_lbn>UINT32_MAX-next_count)
                    return PORT_IOC_MAPVBLK_CORRUPT;
                if(next_lbn==start+count){
                    if(next_count>UINT32_MAX-available)
                        return PORT_IOC_MAPVBLK_CORRUPT;
                    available+=next_count;
                }
            }
            break;
        }
        relative-=count;
    }
    if(!available)return PORT_IOC_MAPVBLK_UNMAPPED;
    if(!(access&PORT_IOC_MAPVBLK_ACCESS_NOTFCP)){
        uint32_t file_blocks=0u;
        if(!file_va||!ops->get_file_size(ops->context,file_va,&file_blocks))
            return PORT_IOC_MAPVBLK_CORRUPT;
        const uint32_t file_vbn=vbn;
        if(file_vbn>file_blocks)return PORT_IOC_MAPVBLK_UNMAPPED;
        const uint32_t eof_blocks=file_blocks-file_vbn+1u;
        if(eof_blocks<available)available=eof_blocks;
    }
    uint64_t mapped=(uint64_t)available*512u;
    if(mapped>=bytes){out->unmapped_bytes=0u;}
    else out->unmapped_bytes=bytes-(uint32_t)mapped;
    const uint8_t rvn=(uint8_t)(lbn>>24);
    if(rvn){
        uint32_t target=0u;
        if(!ops->relative_ucb(ops->context,rd32(wcb+WCB_RVT),rvn,&target)||
           !target)return PORT_IOC_MAPVBLK_BADRVN;
        out->ucb=target;
    }
    out->lbn=lbn&UINT32_C(0x00ffffff);
    return PORT_IOC_MAPVBLK_OK;
}

uint32_t port_ioc_mapvblk(uint32_t vbn,uint32_t bytes,uint32_t window_va,
    uint32_t current_ucb,const struct port_ioc_mapvblk_ops *ops,
    struct port_ioc_mapvblk_result *out){
    if(!ops||!out||!ops->resolve_wcb||!ops->enter||!ops->leave||
       !ops->is_sequential||!ops->get_file_size||!ops->relative_ucb||
       !ops->record_hit||!vbn||!bytes||!window_va||!current_ucb)
        return PORT_IOC_MAPVBLK_BADPARAM;
    result_init(out,bytes,current_ucb);
    uint32_t cookie=0u;
    if(!ops->enter(ops->context,current_ucb,&cookie))
        return PORT_IOC_MAPVBLK_BADPARAM;
    uint32_t selected_va=window_va;
    const uint8_t *selected=0;size_t selected_bytes=0u;
    uint32_t candidate_va=window_va;
    for(unsigned n=0u;n<MAX_WCB_SEGMENTS;n++){
        size_t capacity=0u;
        const uint8_t *candidate=ops->resolve_wcb(ops->context,candidate_va,
            PORT_IOC_MAPVBLK_WCB_BYTES,&capacity);
        if(!candidate||capacity<PORT_IOC_MAPVBLK_WCB_BYTES){
            ops->leave(ops->context,current_ucb,cookie);
            return PORT_IOC_MAPVBLK_BADWCB;
        }
        if(candidate[WCB_TYPE]!=PORT_V37_DYN_WCB){
            ops->leave(ops->context,current_ucb,cookie);
            return PORT_IOC_MAPVBLK_BADWCB;
        }
        const uint32_t start=rd32(candidate+WCB_STVBN);
        if(!selected){selected=candidate;selected_va=candidate_va;
            selected_bytes=capacity;}
        if(vbn<start)break;
        selected=candidate;selected_va=candidate_va;selected_bytes=capacity;
        const uint32_t next=rd32(candidate+WCB_LINK);
        if(!next)break;
        if(next==candidate_va){
            ops->leave(ops->context,current_ucb,cookie);
            return PORT_IOC_MAPVBLK_CORRUPT;
        }
        candidate_va=next;
        if(n+1u==MAX_WCB_SEGMENTS){
            ops->leave(ops->context,current_ucb,cookie);
            return PORT_IOC_MAPVBLK_CORRUPT;
        }
    }
    uint32_t status=map_segment(selected,selected_bytes,vbn,bytes,current_ucb,
                                ops,out);
    if(status==PORT_IOC_MAPVBLK_OK)ops->record_hit(ops->context);
    if(status==PORT_IOC_MAPVBLK_UNMAPPED){
        out->ucb=rd32(selected+WCB_ORGUCB);
        out->unmapped_bytes=bytes;
    }
    (void)selected_va;
    ops->leave(ops->context,current_ucb,cookie);
    return status;
}

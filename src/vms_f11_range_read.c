#include "vms_f11_range.h"
#include "vms_f11_range_private.h"
/* Native FIL$READVBN reads byte ranges from FH2-mapped physical sectors.
 * Caller sees exactly the confirmed prefix on I/O error, reflected in
 * completed; never reports success for a truncated or absent extent. */
uint32_t port_f11_read_range(const uint8_t fh2[512],uint32_t offset,
    uint32_t count,uint8_t *out,size_t capacity,
    port_dq_dir_sector_read_fn reader,void *ctx,uint32_t *completed){
    if(!fh2||!out||!reader||!completed||count>capacity||
       (uint64_t)offset+count>UINT32_MAX)return PORT_F11_RANGE_BAD;
    if(!count){*completed=0u;return PORT_F11_RANGE_OK;}
    const uint32_t rc=port_f11_range_preflight(fh2,offset,count);
    if(rc!=PORT_F11_RANGE_OK)return rc;
    uint32_t done=0u;
    while(done<count){
        const uint32_t at=offset+done;
        const uint32_t vbn=at/512u+1u;
        const uint32_t within=at%512u;
        uint32_t lbn=0u;
        if(port_dq_dir_map_vbn(fh2,vbn,&lbn)!=PORT_DQ_DIR_OK){
            *completed=done;return PORT_F11_RANGE_MAP;
        }
        uint8_t block[512];
        if(reader(ctx,lbn,block)!=PORT_DQ_DIR_OK){
            *completed=done;return PORT_F11_RANGE_IO;
        }
        const uint32_t n=(count-done<512u-within)?count-done:512u-within;
        for(unsigned k=0u;k<n;k++)out[done+k]=block[within+k];
        done+=n;
    }
    *completed=done;
    return PORT_F11_RANGE_OK;
}

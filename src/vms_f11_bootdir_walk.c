#include "vms_f11_bootdir.h"
/* FIL$READVBN maps each one-origin directory VBN separately; the
 * callback must be an actual registered privileged disk-sector reader. */
uint32_t port_f11_bootdir_walk(const struct port_f11_boot_volume *v,
    const uint8_t fh2[512],port_f11_bootdir_visit_fn visit,void *context,
    uint32_t *seen){
    if(!v||!v->read_sector||!fh2||!visit||!seen||
       !v->max_directory_vbn||v->max_directory_vbn>65535u)
        return PORT_F11_BOOTDIR_BAD;
    uint32_t count=0u;
    for(uint32_t vbn=1u;vbn<=v->max_directory_vbn;vbn++){
        uint32_t lbn=0u;
        if(port_dq_dir_map_vbn(fh2,vbn,&lbn)!=PORT_DQ_DIR_OK){
            *seen=count;return PORT_F11_BOOTDIR_MAP;
        }
        uint8_t block[512];
        if(v->read_sector(v->context,lbn,block)!=PORT_DQ_DIR_OK){
            *seen=count;return PORT_F11_BOOTDIR_MAP;
        }
        const uint32_t rc=port_f11_bootdir_block(block,visit,context,&count);
        if(rc!=PORT_F11_BOOTDIR_OK){*seen=count;return rc;}
    }
    *seen=count;
    return PORT_F11_BOOTDIR_OK;
}

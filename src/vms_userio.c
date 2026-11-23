#include "vms_userio.h"
#include "vms_qio.h"
int port_user_io_decode(const struct port_user_map *m,uint32_t descriptor_va,
                        port_phys_page_resolver physical,void *context,
                        struct port_user_io_desc *result) {
    if(!result)return PORT_USER_BAD;
    struct port_user_io_desc temporary={0};
    if(port_user_copy_from(m,&temporary,descriptor_va,sizeof(temporary),
                           physical,context))return PORT_USER_BAD;
    /* Port check accepts the end-inclusive last byte up to UINT32_MAX. */
    if(port_user_range_valid(m,temporary.data_va,512u,1) ||
       port_user_range_valid(m,temporary.iosb_va,
                             sizeof(struct port_qio_iosb),1))return PORT_USER_BAD;
    const uint32_t end_data=temporary.data_va+511u;
    const uint32_t end_iosb=temporary.iosb_va+
                              (uint32_t)sizeof(struct port_qio_iosb)-1u;
    if(temporary.data_va<=end_iosb && temporary.iosb_va<=end_data)
        return PORT_USER_BAD;
    *result=temporary;
    return PORT_USER_OK;
}

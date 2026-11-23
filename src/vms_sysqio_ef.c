#include "vms_sysqio_ef.h"
#include "vms_sys85.h"

/* SYSQIOREQ.MAR 239-247: MOVZBL EFN, clear local bits inline through 63,
 * and call SCH$CLREF for a global event flag. */
uint32_t port_v37_sysqio_clear_efn(uint32_t event_flag,uint32_t local_flags[2],
    port_v37_sysqio_clref_fn clear_global,void *context,
    uint32_t *service_status){
    if(!service_status)return PORT_V37_SYSQIO_EF_BAD;
    const uint8_t efn=(uint8_t)event_flag;
    if(efn<=63u){
        if(!local_flags)return PORT_V37_SYSQIO_EF_BAD;
        const unsigned word=efn>>5u;
        const uint32_t mask=~(UINT32_C(1)<<(efn&31u));
        (void)__atomic_fetch_and(&local_flags[word],mask,__ATOMIC_SEQ_CST);
        *service_status=PORT_SYS85_NORMAL;
        return PORT_V37_SYSQIO_EF_OK;
    }
    if(!clear_global)return PORT_V37_SYSQIO_EF_BAD;
    const uint32_t status=clear_global(context,efn);
    *service_status=status;
    return (status&1u)?PORT_V37_SYSQIO_EF_OK:
                       PORT_V37_SYSQIO_EF_GLOBAL_FAILURE;
}

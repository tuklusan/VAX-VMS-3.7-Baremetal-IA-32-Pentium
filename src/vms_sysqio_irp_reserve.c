#include "vms_sysqio_args.h"
#include "vms_exe_schevt.h"
#include "vms_sys85.h"

/* SYSQIOREQ.MAR 339-345, 386-429 and 537-540: EXE$ALLOCIRP and IRP/ASTCNT
 * construction precede AST quota validation. On rejection this isolated
 * transaction rolls the AST charge back and returns the packet; channel IOC,
 * IOSB/event and process post work remain the caller's EXE$ABORTIO duties. */
uint32_t port_v37_sysqio_reserve_irp(
    const struct port_v37_sysqio_prepared *request,
    struct port_v37_sysqio_process *process,
    const struct port_v37_sysqio_irp_spec *spec,
    struct port_ini_packet_list *pool,
    const struct port_v37_irp_pool_sync *sync,
    uint8_t **packet,uint32_t *packet_va,uint32_t *service_status){
    if(!request||!process||!spec||!pool||!sync||!packet||!packet_va||
       !service_status)return PORT_V37_SYSQIO_RESERVE_BAD;
    *packet=0;*packet_va=0;
    uint8_t *allocated=0;uint32_t va=0;
    const uint32_t alloc=port_v37_irp_pool_alloc(pool,
        PORT_V37_SYSQIO_IRP_STANDARD_BYTES,sync,&allocated,&va);
    if(alloc!=PORT_V37_IRP_POOL_OK){
        *service_status=alloc==PORT_V37_IRP_POOL_EMPTY?
            PORT_SS_INSFMEM:PORT_SYS85_BADPARAM;
        return alloc==PORT_V37_IRP_POOL_EMPTY?
            PORT_V37_SYSQIO_RESERVE_EMPTY:PORT_V37_SYSQIO_RESERVE_POOL;
    }
    const int16_t ast_count_before=process->ast_count;
    struct port_v37_sysqio_irp_spec actual=*spec;
    actual.packet_va=va;
    if(port_v37_sysqio_build_irp(request,process,&actual,allocated,
          pool->packet_bytes)!=PORT_V37_SYSQIO_IRP_BUILD_OK){
        const uint32_t returned=port_v37_irp_pool_free(pool,allocated,va,
            pool->packet_bytes,sync);
        *service_status=PORT_SYS85_BADPARAM;
        return returned==PORT_V37_IRP_POOL_OK?
            PORT_V37_SYSQIO_RESERVE_BAD:PORT_V37_SYSQIO_RESERVE_POOL;
    }
    uint32_t ast_status=PORT_SYS85_NORMAL;
    if(port_v37_sysqio_check_ast_quota(request,process,&ast_status)!=
       PORT_V37_SYSQIO_AST_OK){
        process->ast_count=ast_count_before;
        const uint32_t irp_bytes=(uint32_t)allocated[8u]|
            ((uint32_t)allocated[9u]<<8);
        const uint32_t returned=port_v37_irp_pool_free(pool,allocated,va,
            irp_bytes,sync);
        *service_status=ast_status;
        return returned==PORT_V37_IRP_POOL_OK?
            PORT_V37_SYSQIO_RESERVE_ASTQUOTA:PORT_V37_SYSQIO_RESERVE_POOL;
    }
    *packet=allocated;*packet_va=va;*service_status=PORT_SYS85_NORMAL;
    return PORT_V37_SYSQIO_RESERVE_OK;
}

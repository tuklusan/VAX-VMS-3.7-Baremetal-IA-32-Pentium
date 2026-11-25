#include "vms_sysqio_args.h"
#include "vms_exe_schevt.h"
#include "vms_sys85.h"

enum { IRP_STS=42u,IRP_STS_PHYSIO=0x0100u };

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
    const int16_t ast_count_before=port_v37_sysqio_process_ast_count(process);
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
        port_v37_sysqio_process_set_ast_count(process,ast_count_before);
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

/* SYSQIOREQ.MAR ALLOC/ERROR ordering: if lookaside allocation or AST quota
 * fails after NOIOSB charged the process, back out that exact charge and post
 * the error event. Outputs become visible only after the complete reservation. */
uint32_t port_v37_sysqio_prepare_irp(const void *ap,size_t ap_bytes,
    struct port_v37_sysqio_process *process,
    const struct port_v37_sysqio_begin_ops *ops,
    const struct port_v37_sysqio_irp_spec *spec,
    struct port_ini_packet_list *pool,const struct port_v37_irp_pool_sync *sync,
    struct port_v37_sysqio_prepared *prepared,uint8_t **packet,
    uint32_t *packet_va,uint32_t *service_status){
    if(!process||!ops||!spec||!pool||!sync||!prepared||!packet||!packet_va||
       !service_status)return PORT_V37_SYSQIO_PREPARE_BEGIN;
    struct port_v37_sysqio_prepared request;
    uint32_t status=PORT_SYS85_BADPARAM;
    const uint32_t begun=port_v37_sysqio_begin(ap,ap_bytes,process,ops,
                                                &request,&status);
    if(begun!=PORT_V37_SYSQIO_BEGIN_OK){
        if(begun==PORT_V37_SYSQIO_BEGIN_WAIT)
            return PORT_V37_SYSQIO_PREPARE_WAIT;
        /* No IRP ownership is returned on a rejected begin. Clear stale
         * caller outputs, while preserving the WAIT path's no-op contract. */
        *packet=0;*packet_va=0u;
        *service_status=status;
        return PORT_V37_SYSQIO_PREPARE_BEGIN;
    }
    const int16_t ast_count_before=port_v37_sysqio_process_ast_count(process);
    uint8_t *reserved=0;uint32_t va=0;
    const uint32_t result=port_v37_sysqio_reserve_irp(&request,process,spec,
        pool,sync,&reserved,&va,&status);
    if(result!=PORT_V37_SYSQIO_RESERVE_OK){
        *packet=0;*packet_va=0u;
        const int16_t count=port_v37_sysqio_process_io_count(process,
            request.dispatch.buffered);
        if(count<INT16_MAX)
            port_v37_sysqio_process_set_io_count(process,
                request.dispatch.buffered,(int16_t)(count+1));
        if(ops->post_event)ops->post_event(ops->context,process->pid,
            (uint8_t)request.args.event_flag,status);
        *service_status=status;
        return PORT_V37_SYSQIO_PREPARE_RESERVE;
    }
    /* SYSQIOREQ.MAR ACCESS runs after the standard IRP/AST reservation.
     * Preserve the source order: privilege failure returns the IRP and both
     * process charges before the request becomes visible to the caller. */
    uint8_t physical_io=0u;
    const uint32_t access=port_v37_sysqio_device_access(
        request.args.function,request.channel.device->characteristics,
        process->has_phy_io,process->has_log_io,
        request.channel.device->volume_protection,
        request.channel.device->owner_uic,process->arb_va,
        request.channel.device->check_volume,
        request.channel.device->volume_context,&physical_io,&status);
    if(access!=PORT_V37_SYSQIO_DEVICE_OK){
        if(request.args.ast_address)
            port_v37_sysqio_process_set_ast_count(process,ast_count_before);
        const uint32_t packet_bytes=(uint32_t)reserved[8u]|
            ((uint32_t)reserved[9u]<<8);
        const uint32_t returned=port_v37_irp_pool_free(pool,reserved,va,
            packet_bytes,sync);
        const int16_t count=port_v37_sysqio_process_io_count(process,
            request.dispatch.buffered);
        if(count<INT16_MAX)
            port_v37_sysqio_process_set_io_count(process,
                request.dispatch.buffered,(int16_t)(count+1));
        if(ops->post_event)ops->post_event(ops->context,process->pid,
            (uint8_t)request.args.event_flag,status);
        *service_status=status;
        *packet=0;*packet_va=0u;
        return returned==PORT_V37_IRP_POOL_OK?
            PORT_V37_SYSQIO_PREPARE_ACCESS:PORT_V37_SYSQIO_PREPARE_RESERVE;
    }
    if(physical_io){
        const uint16_t st=(uint16_t)reserved[IRP_STS]|
            (uint16_t)((uint16_t)reserved[IRP_STS+1u]<<8);
        reserved[IRP_STS]=(uint8_t)(st|IRP_STS_PHYSIO);
        reserved[IRP_STS+1u]=(uint8_t)((st|IRP_STS_PHYSIO)>>8);
    }
    request.dispatch.physical_io=physical_io;
    *prepared=request;*packet=reserved;*packet_va=va;
    *service_status=status;
    return PORT_V37_SYSQIO_PREPARE_OK;
}

uint32_t port_v37_sysqio_prepare_user_irp(
    const struct port_exe_access *memory,uint32_t ap_address,
    struct port_v37_sysqio_process *process,
    const struct port_v37_sysqio_begin_ops *ops,
    const struct port_v37_sysqio_irp_spec *spec,
    struct port_ini_packet_list *pool,
    const struct port_v37_irp_pool_sync *sync,
    struct port_v37_sysqio_prepared *prepared,uint8_t **packet,
    uint32_t *packet_va,uint32_t *service_status){
    if(!service_status||!packet||!packet_va)return PORT_V37_SYSQIO_PREPARE_BEGIN;
    *packet=0;*packet_va=0u;*service_status=PORT_SYS85_BADPARAM;
    if(!memory||!ap_address||!process||!ops||!spec||!pool||!sync||!prepared)
        return PORT_V37_SYSQIO_PREPARE_BEGIN;
    _Alignas(4) uint8_t ap[PORT_V37_SYSQIO_LIST_BYTES]={0};
    if(!port_exe_probe_read(memory,ap_address,4u)||
       !port_exe_read(memory,ap_address,ap,4u)){
        *service_status=PORT_SYS85_ACCVIO;
        return PORT_V37_SYSQIO_PREPARE_BEGIN;
    }
    const uint32_t count=(uint32_t)ap[0]|((uint32_t)ap[1]<<8)|
        ((uint32_t)ap[2]<<16)|((uint32_t)ap[3]<<24);
    if(count<PORT_V37_SYSQIO_REQUIRED_ARGS)
        return port_v37_sysqio_prepare_irp(ap,4u,process,ops,spec,pool,sync,
            prepared,packet,packet_va,service_status);
    if(!port_exe_probe_read(memory,ap_address,sizeof ap)||
       !port_exe_read(memory,ap_address,ap,sizeof ap)){
        *service_status=PORT_SYS85_ACCVIO;
        return PORT_V37_SYSQIO_PREPARE_BEGIN;
    }
    struct port_v37_sysqio_prepared request={0};
    uint8_t *reserved=0;uint32_t va=0u;uint32_t status=PORT_SYS85_BADPARAM;
    const int16_t ast_before=port_v37_sysqio_process_ast_count(process);
    const uint32_t result=port_v37_sysqio_prepare_irp(ap,sizeof ap,process,
        ops,spec,pool,sync,&request,&reserved,&va,&status);
    if(result!=PORT_V37_SYSQIO_PREPARE_OK){
        *service_status=status;
        return result;
    }
    struct port_v37_sysqio_buffer buffer={0};
    int pin_failed=0;
    if(request.args.iosb&&!port_exe_lock_io(memory,request.args.iosb,
        PORT_V37_SYSQIO_IOSB_BYTES,1,&request.iosb_pages)){
        pin_failed=1;status=PORT_SYS85_ACCVIO;
    }
    const uint32_t checked=pin_failed?PORT_V37_SYSQIO_BUFFER_ACCVIO:
        port_v37_sysqio_check_buffer(&request,memory,&buffer,&status);
    if((checked==PORT_V37_SYSQIO_BUFFER_OK)&&buffer.bytes){
        const int writing=buffer.direction==
            PORT_V37_SYSQIO_BUFFER_WRITE_TO_USER;
        if(!port_exe_lock_io(memory,buffer.address,buffer.bytes,
            writing,&request.transfer_pages)){
            pin_failed=1;status=PORT_SYS85_ACCVIO;
        }
    }
    if((checked!=PORT_V37_SYSQIO_BUFFER_OK&&
        checked!=PORT_V37_SYSQIO_BUFFER_NONE)||pin_failed){
        /* prepare_irp charged I/O and AST quotas and owns a private packet.
         * The access-fault path mirrors EXE$ABORTIO: return the packet and
         * restore both process charges before posting the event flag. */
        const uint32_t bytes=(uint32_t)reserved[8u]|
            ((uint32_t)reserved[9u]<<8);
        const uint32_t returned=port_v37_irp_pool_free(pool,reserved,va,
            bytes,sync);
        port_user_unpin_io_range(&request.transfer_pages);
        port_user_unpin_io_range(&request.iosb_pages);
        if(returned!=PORT_V37_IRP_POOL_OK){
            *service_status=PORT_SYS85_BADPARAM;
            *packet=reserved;*packet_va=va;
            return PORT_V37_SYSQIO_PREPARE_RESERVE;
        }
        if(request.args.ast_address)
            port_v37_sysqio_process_set_ast_count(process,ast_before);
        const int16_t io=port_v37_sysqio_process_io_count(process,
            request.dispatch.buffered);
        if(io<INT16_MAX)
            port_v37_sysqio_process_set_io_count(process,
                request.dispatch.buffered,(int16_t)(io+1));
        if(ops->post_event)ops->post_event(ops->context,process->pid,
            (uint8_t)request.args.event_flag,status);
        *service_status=status;
        return PORT_V37_SYSQIO_PREPARE_BUFFER;
    }
    *prepared=request;*packet=reserved;*packet_va=va;
    *service_status=status;
    return PORT_V37_SYSQIO_PREPARE_OK;
}

void port_v37_sysqio_release_user_pages(
    struct port_v37_sysqio_prepared *request){
    if(request){
        port_user_unpin_io_range(&request->transfer_pages);
        port_user_unpin_io_range(&request->iosb_pages);
    }
}

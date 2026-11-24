#include "vms_sysqio_args.h"
#include "vms_qio_post.h"
#include "vms_v37_dyn_tags.h"

enum { IRP_SIZE=8u,IRP_TYPE=10u,IRP_RMOD=11u,IRP_PID=12u,
       IRP_AST=16u,IRP_ASTPRM=20u,IRP_WIND=24u,IRP_UCB=28u,
       IRP_FUNC=32u,IRP_EFN=34u,IRP_PRI=35u,IRP_IOSB=36u,
       IRP_CHAN=40u,IRP_STS=42u,IRP_SVAPTE=44u,IRP_BOFF=48u,
       IRP_BCNT=52u,IRP_STS_BUFIO=1u,IRP_RMOD_QUOTA=0x40u };
_Static_assert(PORT_V37_IRP_ARB+4u<=PORT_V37_SYSQIO_IRP_STANDARD_BYTES,
               "standard IRP includes ARB");
_Static_assert((unsigned)PORT_V37_IRP_SIZE==
               (unsigned)PORT_V37_SYSQIO_IRP_STANDARD_BYTES,
               "SYSQIO IRP size agrees with resident IRP ABI");

static void put16(uint8_t *p,uint16_t v){
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
}
static void put32(uint8_t *p,uint32_t v){
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}

/* SYSQIOREQ.MAR SUCCES, SYSDEF.MDL IRP prefix and CDRP reservation. The
 * packet is built off-queue so malformed inputs cannot leave a partial IRP. */
uint32_t port_v37_sysqio_build_irp(
    const struct port_v37_sysqio_prepared *request,
    struct port_v37_sysqio_process *process,
    const struct port_v37_sysqio_irp_spec *spec,
    uint8_t *packet,size_t packet_bytes){
    if(!request||!process||!spec||!packet||!process->pid||
       process->access_mode>3u||packet_bytes<PORT_V37_SYSQIO_IRP_STANDARD_BYTES||
       packet_bytes>UINT16_MAX||(spec->packet_va&3u)||!spec->packet_va||
       (spec->ucb_va&3u)||!spec->ucb_va||(spec->arb_va&3u)||
       (spec->window_va&3u)||spec->channel_index>UINT16_MAX||
       spec->base_priority>31u||
       request->args.function!=request->dispatch.full_function)
        return PORT_V37_SYSQIO_IRP_BUILD_BAD;
    uint8_t candidate[PORT_V37_SYSQIO_IRP_STANDARD_BYTES]={0};
    put16(candidate+IRP_SIZE,PORT_V37_SYSQIO_IRP_STANDARD_BYTES);
    candidate[IRP_TYPE]=PORT_V37_DYN_IRP;
    candidate[IRP_RMOD]=(uint8_t)(process->access_mode&3u);
    if(request->args.ast_address)candidate[IRP_RMOD]|=IRP_RMOD_QUOTA;
    put32(candidate+IRP_PID,process->pid);
    put32(candidate+IRP_AST,request->args.ast_address);
    put32(candidate+IRP_ASTPRM,request->args.ast_parameter);
    put32(candidate+IRP_WIND,spec->window_va);
    put32(candidate+IRP_UCB,spec->ucb_va);
    put16(candidate+IRP_FUNC,request->dispatch.full_function);
    candidate[IRP_EFN]=(uint8_t)request->args.event_flag;
    candidate[IRP_PRI]=spec->base_priority;
    put32(candidate+IRP_IOSB,request->args.iosb);
    put16(candidate+IRP_CHAN,(uint16_t)spec->channel_index);
    put16(candidate+IRP_STS,request->dispatch.buffered?IRP_STS_BUFIO:0u);
    put32(candidate+IRP_SVAPTE,0u);
    put16(candidate+IRP_BOFF,0u);
    put32(candidate+IRP_BCNT,0u);
    put32(candidate+PORT_V37_IRP_ARB,spec->arb_va);
    if(request->args.ast_address){
        /* VMS DECW semantics without signed C overflow. */
        const int16_t ast_count=port_v37_sysqio_process_ast_count(process);
        port_v37_sysqio_process_set_ast_count(process,
            ast_count==INT16_MIN?INT16_MAX:(int16_t)(ast_count-1));
    }
    for(size_t i=0;i<sizeof candidate;i++)packet[i]=candidate[i];
    return PORT_V37_SYSQIO_IRP_BUILD_OK;
}

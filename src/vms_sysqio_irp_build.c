#include "vms_sysqio_args.h"
#include "vms_qio_post.h"
#include "vms_v37_dyn_tags.h"
#include "vms_v37_pcb.h"

enum { IRP_SIZE=8u,IRP_TYPE=10u,IRP_RMOD=11u,IRP_PID=12u,
       IRP_AST=16u,IRP_ASTPRM=20u,IRP_WIND=24u,IRP_UCB=28u,
       IRP_FUNC=32u,IRP_EFN=34u,IRP_PRI=35u,IRP_IOSB=36u,
       IRP_CHAN=40u,IRP_STS=42u,IRP_SVAPTE=44u,IRP_BOFF=48u,
       IRP_BCNT=52u,IRP_CARCON=64u,IRP_STS_BUFIO=1u,
       IRP_STS_FUNC=2u,IRP_RMOD_QUOTA=0x40u };
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
static uint16_t get16(const uint8_t *p){
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));
}
static uint32_t get32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
        ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}

/* SYSQIOREQ.MAR SUCCES, SYSDEF.MDL IRP prefix and CDRP reservation. The
 * packet is built off-queue so malformed inputs cannot leave a partial IRP. */
uint32_t port_v37_sysqio_build_irp(
    const struct port_v37_sysqio_prepared *request,
    struct port_v37_sysqio_process *process,
    const struct port_v37_sysqio_irp_spec *spec,
    uint8_t *packet,size_t packet_bytes){
    const uint32_t source_channel=request?
        (request->args.channel&UINT32_C(0x0000fff0)):0u;
    uint8_t request_priority=spec?spec->base_priority:0u;
    if(!request||!process||!spec||!packet||!process->pid||
       process->access_mode>3u||packet_bytes<PORT_V37_SYSQIO_IRP_STANDARD_BYTES||
       packet_bytes>UINT16_MAX||(spec->packet_va&3u)||!spec->packet_va||
       (spec->ucb_va&3u)||!spec->ucb_va||(spec->arb_va&3u)||
       (spec->window_va&3u)||source_channel==0u||
       spec->channel_index!=source_channel||
       spec->channel_index==0u||
       spec->channel_index>UINT16_MAX||
       (spec->channel_index%PORT_V37_CCB_BYTES)!=0u||
       spec->base_priority>31u||
       request->args.function!=request->dispatch.full_function)
        return PORT_V37_SYSQIO_IRP_BUILD_BAD;
    if(process->source_pcb){
        /* SYSQIOREQ builds IRP$B_PRI from PCB$B_PRIB (the base, inverted
         * VMS priority), not from a caller supplied dispatch parameter. */
        if(process->source_pcb_bytes<PORT_V37_PCB_BYTES||
           get16(process->source_pcb+PORT_V37_PCB_SIZE)!=
               PORT_V37_PCB_BYTES||
           process->source_pcb[PORT_V37_PCB_BTYPE]!=PORT_V37_DYN_PCB||
           get32(process->source_pcb+PORT_V37_PCB_PID)!=
               process->pid||
           process->source_pcb[PORT_V37_PCB_PRIB]>31u)
            return PORT_V37_SYSQIO_IRP_BUILD_BAD;
        request_priority=process->source_pcb[PORT_V37_PCB_PRIB];
    }
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
    candidate[IRP_PRI]=request_priority;
    put32(candidate+IRP_IOSB,request->args.iosb);
    /* SYSQIOREQ.VCHAN negates the byte-offset channel index in R9.
     * IRP$W_CHAN therefore holds the negative CCB offset; IOCIOPOST
     * sign-extends it and adds it to CTL$GL_CCBBASE. */
    put16(candidate+IRP_CHAN,
          (uint16_t)(0u-(uint16_t)spec->channel_index));
    uint16_t irp_status=request->dispatch.buffered?IRP_STS_BUFIO:0u;
    if(request->dispatch.action_class==PORT_DQ_FDT_READ)
        irp_status|=IRP_STS_FUNC;
    put16(candidate+IRP_STS,irp_status);
    put32(candidate+IRP_SVAPTE,0u);
    put16(candidate+IRP_BOFF,0u);
    put32(candidate+IRP_BCNT,0u);
    put32(candidate+PORT_V37_IRP_ARB,spec->arb_va);
    if(request->dispatch.action_class==PORT_DQ_FDT_READ||
       request->dispatch.action_class==PORT_DQ_FDT_WRITE){
        /* SYSQIOFDT EXE$READ/WRITE common path stores P2 as a
         * zero-extended transfer byte count and P4 as carriage control.
         * User-buffer accessibility, page locking and SVAPTE binding are
         * still required before this IRP can be submitted to a driver. */
        put32(candidate+IRP_BCNT,request->args.parameter[1]);
        candidate[IRP_CARCON]=(uint8_t)request->args.parameter[3];
    }
    /* SYSQIOFDT.MAR EXE$ONEPARM stores P1 in IRP$L_MEDIA before queuing.
     * Keep that FDT action in the standard packet so DQ SEEK receives P1
     * through the same IRP field used by its original driver. */
    if(request->dispatch.action_class==PORT_DQ_FDT_ONE)
        put32(candidate+PORT_V37_IRP_MEDIA,request->args.parameter[0]);
    if(request->args.ast_address){
        /* VMS DECW semantics without signed C overflow. */
        const int16_t ast_count=port_v37_sysqio_process_ast_count(process);
        port_v37_sysqio_process_set_ast_count(process,
            ast_count==INT16_MIN?INT16_MAX:(int16_t)(ast_count-1));
    }
    for(size_t i=0;i<sizeof candidate;i++)packet[i]=candidate[i];
    return PORT_V37_SYSQIO_IRP_BUILD_OK;
}


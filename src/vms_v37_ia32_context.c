#include "vms_v37_ia32_context.h"

_Static_assert(offsetof(struct port_v37_ia32_restore_frame,registers)==16u,
               "arch_isr_common segment-save prefix");
_Static_assert(sizeof(struct port_v37_ia32_restore_frame)==76u,
               "arch_isr_common saved context size");

uint32_t port_v37_ia32_context_init(void *kernel_stack,size_t stack_bytes,
    const struct port_v37_ia32_context_seed *seed,
    struct port_v37_ia32_restore_frame **restore_frame){
    if(!kernel_stack||!seed||!restore_frame||seed->reserved||!seed->cr3||
       (seed->cr3&4095u)||!seed->entry_pc||
       (seed->code_selector&3u)!=seed->access_mode||
       (seed->data_selector&3u)!=seed->access_mode||
       !seed->code_selector||!seed->data_selector)
        return PORT_V37_IA32_CONTEXT_BAD;
    if(seed->access_mode>3u)
        return PORT_V37_IA32_CONTEXT_PRIVILEGE;
    static const uint16_t code_selector[4]={0x08u,0x19u,0x2au,0x3bu};
    static const uint16_t data_selector[4]={0x10u,0x21u,0x32u,0x43u};
    if(seed->code_selector!=code_selector[seed->access_mode]||
       seed->data_selector!=data_selector[seed->access_mode])
        return PORT_V37_IA32_CONTEXT_PRIVILEGE;
    if((seed->eflags&UINT32_C(0x00027000))||!(seed->eflags&2u))
        return PORT_V37_IA32_CONTEXT_FLAGS;
    if(stack_bytes>UINT32_MAX||
       stack_bytes<sizeof(struct port_v37_ia32_restore_frame)+32u||
       ((uintptr_t)kernel_stack&15u))
        return PORT_V37_IA32_CONTEXT_STACK;
    if(seed->access_mode==0u){
        if(seed->initial_stack)
            return PORT_V37_IA32_CONTEXT_STACK;
    }else if(!seed->initial_stack){
        return PORT_V37_IA32_CONTEXT_STACK;
    }
    const uintptr_t top=(uintptr_t)kernel_stack+stack_bytes;
    struct port_v37_ia32_restore_frame *const frame=
        (struct port_v37_ia32_restore_frame *)
            ((top-sizeof(*frame))&~(uintptr_t)15u);
    const struct port_v37_ia32_restore_frame candidate={
        .gs=seed->data_selector,.fs=seed->data_selector,
        .es=seed->data_selector,.ds=seed->data_selector,
        .registers={
            .edi=0u,.esi=0u,.ebp=0u,.esp_at_pushal=0u,
            .ebx=0u,.edx=0u,.ecx=0u,.eax=0u,
            .vector=0u,.error=0u,.eip=seed->entry_pc,
            .cs=seed->code_selector,.eflags=seed->eflags,
            .user_esp=seed->access_mode?seed->initial_stack:0u,
            .user_ss=seed->access_mode?seed->data_selector:0u
        }
    };
    *frame=candidate;
    *restore_frame=frame;
    return PORT_V37_IA32_CONTEXT_OK;
}

uint32_t port_v37_ia32_context_validate(
    const struct port_v37_ia32_restore_frame *frame){
    if(!frame||((uintptr_t)frame&3u)||!frame->registers.eip)
        return PORT_V37_IA32_CONTEXT_BAD;
    static const uint16_t code[4]={0x08u,0x19u,0x2au,0x3bu};
    static const uint16_t data[4]={0x10u,0x21u,0x32u,0x43u};
    const uint8_t mode=(uint8_t)(frame->registers.cs&3u);
    if(frame->registers.cs!=code[mode]||frame->ds!=data[mode]||
       frame->es!=data[mode]||frame->fs!=data[mode]||frame->gs!=data[mode])
        return PORT_V37_IA32_CONTEXT_PRIVILEGE;
    if((frame->registers.eflags&UINT32_C(0x00027000))||
       !(frame->registers.eflags&2u))
        return PORT_V37_IA32_CONTEXT_FLAGS;
    if(mode==0u){
        if(frame->registers.user_esp||frame->registers.user_ss)
            return PORT_V37_IA32_CONTEXT_STACK;
    }else if(!frame->registers.user_esp||
             frame->registers.user_ss!=data[mode]){
        return PORT_V37_IA32_CONTEXT_STACK;
    }
    return PORT_V37_IA32_CONTEXT_OK;
}

uint32_t port_v37_ia32_context_stack_top(uintptr_t frame_address,
    size_t frame_bytes,uint32_t *stack_top){
    if(!stack_top||!frame_bytes||frame_address>UINT32_MAX||
       frame_bytes>UINT32_MAX||frame_address>UINT32_MAX-frame_bytes)
        return PORT_V37_IA32_CONTEXT_STACK;
    *stack_top=(uint32_t)(frame_address+frame_bytes);
    return PORT_V37_IA32_CONTEXT_OK;
}

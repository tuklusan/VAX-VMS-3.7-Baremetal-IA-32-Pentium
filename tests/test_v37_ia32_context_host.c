#include "vms_v37_ia32_context.h"
#include <stdio.h>
#include <string.h>

static int fail(const char *s){fprintf(stderr,"IA32 CONTEXT FAIL: %s\n",s);return 1;}

int main(void){
    uint32_t stack_top=0u;
    if(port_v37_ia32_context_stack_top(UINT32_MAX-76u,76u,&stack_top)!=
       PORT_V37_IA32_CONTEXT_OK||stack_top!=UINT32_MAX)
        return fail("32-bit TSS stack top at address limit");
    if(port_v37_ia32_context_stack_top(UINT32_MAX-75u,76u,&stack_top)!=
       PORT_V37_IA32_CONTEXT_STACK)
        return fail("TSS stack extent rejects 32-bit wrap");
    if(port_v37_ia32_context_stack_top(0x100000000ull,76u,&stack_top)!=
       PORT_V37_IA32_CONTEXT_STACK)
        return fail("TSS stack address must fit IA-32");
    _Alignas(16) uint8_t stack[512];
    memset(stack,0xa5,sizeof stack);
    struct port_v37_ia32_restore_frame *frame=0;
    const struct port_v37_ia32_context_seed kernel={
        .cr3=0x00123000u,.entry_pc=0x00101000u,.eflags=0x202u,
        .code_selector=0x08u,.data_selector=0x10u,.access_mode=0u};
    const uint32_t kernel_status=port_v37_ia32_context_init(stack,sizeof stack,
        &kernel,&frame);
    if(kernel_status!=PORT_V37_IA32_CONTEXT_OK||!frame||
       (uintptr_t)frame<(uintptr_t)stack||
       (uintptr_t)frame+sizeof(*frame)>(uintptr_t)stack+sizeof stack||
       ((uintptr_t)frame&15u)||frame->gs!=0x10u||frame->fs!=0x10u||
       frame->es!=0x10u||frame->ds!=0x10u||
       frame->registers.eip!=kernel.entry_pc||
       frame->registers.cs!=kernel.code_selector||
       frame->registers.eflags!=kernel.eflags||
       frame->registers.user_esp||frame->registers.user_ss){
        fprintf(stderr,"status=%u frame=%p",kernel_status,(void *)frame);
        if(frame)fprintf(stderr," gs=%x fs=%x es=%x ds=%x eip=%x cs=%x flags=%x useresp=%x userss=%x\n",
            frame->gs,frame->fs,frame->es,frame->ds,
            frame->registers.eip,frame->registers.cs,
            frame->registers.eflags,frame->registers.user_esp,
            frame->registers.user_ss);
        return fail("initial supervisor IRET frame");
    }

    const struct port_v37_ia32_context_seed user={
        .cr3=0x00200000u,.entry_pc=0x00401000u,
        .initial_stack=0x00800000u,.eflags=0x202u,
        .code_selector=0x3bu,.data_selector=0x43u,.access_mode=3u};
    if(port_v37_ia32_context_init(stack,sizeof stack,&user,&frame)!=
       PORT_V37_IA32_CONTEXT_OK||frame->registers.cs!=0x3bu||
       frame->registers.user_ss!=0x43u||
       frame->registers.user_esp!=0x00800000u)
        return fail("initial user IRET frame");

    const struct port_v37_ia32_context_seed ring1={
        .cr3=0x00200000u,.entry_pc=0x00401000u,
        .initial_stack=0x007ff000u,.eflags=0x202u,
        .code_selector=0x19u,.data_selector=0x21u,.access_mode=1u};
    if(port_v37_ia32_context_init(stack,sizeof stack,&ring1,&frame)!=
       PORT_V37_IA32_CONTEXT_OK||frame->registers.cs!=0x19u||
       frame->registers.user_ss!=0x21u||
       frame->registers.user_esp!=0x007ff000u)
        return fail("VMS executive mode maps to CPL1");
    frame->registers.vector=32u;
    if(port_v37_ia32_context_validate(frame)!=PORT_V37_IA32_CONTEXT_OK)
        return fail("CPL1 interrupt enters the CPL0 handler with saved ESP/SS");
    frame->registers.user_ss=0x12345678u;
    if(port_v37_ia32_context_validate(frame)!=PORT_V37_IA32_CONTEXT_STACK)
        return fail("CPL1 interrupt frame retains the privilege-transition stack");
    struct port_v37_ia32_context_seed invalid_ring=ring1;
    invalid_ring.code_selector=0x09u;
    if(port_v37_ia32_context_init(stack,sizeof stack,&invalid_ring,&frame)!=
       PORT_V37_IA32_CONTEXT_PRIVILEGE)
        return fail("CPL1 selector must name the CPL1 descriptor");
    const struct port_v37_ia32_context_seed supervisor={
        .cr3=0x00300000u,.entry_pc=0x00402000u,
        .initial_stack=0x007fe000u,.eflags=0x202u,
        .code_selector=0x2au,.data_selector=0x32u,.access_mode=2u};
    if(port_v37_ia32_context_init(stack,sizeof stack,&supervisor,&frame)!=
       PORT_V37_IA32_CONTEXT_OK||frame->registers.cs!=0x2au||
       frame->registers.user_ss!=0x32u||
       frame->registers.user_esp!=0x007fe000u)
        return fail("VMS supervisor mode maps to CPL2");
    frame->registers.vector=46u;
    if(port_v37_ia32_context_validate(frame)!=PORT_V37_IA32_CONTEXT_OK)
        return fail("CPL2 interrupt enters the CPL0 handler with saved ESP/SS");
    struct port_v37_ia32_context_seed invalid=user;
    invalid.eflags|=0x3000u;
    if(port_v37_ia32_context_init(stack,sizeof stack,&invalid,&frame)!=
       PORT_V37_IA32_CONTEXT_FLAGS)
        return fail("user cannot seed IOPL");
    invalid=user;invalid.initial_stack=0u;
    if(port_v37_ia32_context_init(stack,sizeof stack,&invalid,&frame)!=
       PORT_V37_IA32_CONTEXT_STACK)
        return fail("user context requires a stack");
    invalid=user;invalid.cr3|=1u;
    if(port_v37_ia32_context_init(stack,sizeof stack,&invalid,&frame)!=
       PORT_V37_IA32_CONTEXT_BAD)
        return fail("unaligned CR3 rejected");
    puts("IA-32 VMS access-mode 0-3 initial IRET context ABI PASS");
    return 0;
}

#include "vms_exe_scbvector.h"
#include <stddef.h>
/* Named SCB entries through 0xff. Slots >= 0x100 are unused nexus vectors. */
static const struct {
    const char *name;
    uint8_t interrupt_stack;
    uint8_t pc_idt;
} native_slots[64]={
    {0,1,255},{"EXE$MCHK",1,255},{"EXE$KERSTKNV",1,8},{"EXE$POWERFAIL",1,255},
    {"EXE$OPCDEC",0,6},{"EXE$OPCCUS",0,255},{"EXE$ROPRAND",0,255},
    {"EXE$RADRMOD",0,255},{"EXE$ACVIOLAT",0,13},{"MMG$PAGEFAULT",0,14},
    {"EXE$TBIT",0,1},{"EXE$BREAK",0,3},{"EXE$COMPAT",0,255},
    {"EXE$ARITH",0,0},{0,0,255},{0,0,255},
    {"EXE$CMODKRNL",0,128},{"EXE$CMODEXEC",0,255},
    {"EXE$CMODSUPR",0,255},{"EXE$CMODUSER",0,255},
    {0,0,255},{"EXE$INT54",1,255},{"EXE$INT58",1,255},
    {"EXE$INT5C",1,255},{"EXE$INT60",1,255},
    {0,0,255},{0,0,255},{0,0,255},{0,0,255},
    {0,0,255},{0,0,255},{0,0,255},{0,0,255},
    {0,1,255},{"SCH$ASTDEL",0,255},{"SCH$RESCHED",0,255},
    {"IOC$IOPOST",1,255},{"INI$MASTERWAKE",1,255},
    {"EXE$FRKIPL6DSP",1,255},{"EXE$SWTIMINT",1,255},
    {"EXE$FRKIPL8DSP",1,255},{"EXE$FRKIPL9DSP",1,255},
    {"EXE$FRKIPL10DSP",1,255},{"EXE$FRKIPL11DSP",1,255},
    {"EXE$IPCONTROL",1,255},{0,1,255},{0,1,255},
    {0,1,255},{"EXE$HWCLKINT",1,32},
    {0,1,255},{0,1,255},{0,1,255},{0,1,255},
    {0,1,255},{0,1,255},{0,1,255},{0,1,255},
    {0,1,255},{0,1,255},{0,1,255},{0,1,255},
    {0,1,255},{"CON$INTDISI",1,36},{"CON$INTDISO",1,36}
};
_Static_assert(sizeof(native_slots)/sizeof(native_slots[0])==64,"SCB section length");
void port_exe_scb_init(struct port_exe_scb_slot slots[PORT_SCB_ENTRIES]){
    if(!slots)return;
    for(unsigned i=0;i<PORT_SCB_ENTRIES;i++){
        slots[i].vax_byte_offset=(uint16_t)(i<<2);
        if(i<64u){
            slots[i].original_handler=native_slots[i].name;
            slots[i].interrupt_stack=native_slots[i].interrupt_stack;
            slots[i].pc_idt_vector=native_slots[i].pc_idt;
            slots[i].unused=(uint8_t)(native_slots[i].name==NULL);
        } else {
            slots[i].original_handler=NULL;
            slots[i].interrupt_stack=1;
            slots[i].pc_idt_vector=PORT_SCB_NO_PC_VECTOR;
            slots[i].unused=1;
        }
    }
}
const struct port_exe_scb_slot *port_exe_scb_get(const struct port_exe_scb_slot slots[PORT_SCB_ENTRIES],uint16_t original_offset){
    if(!slots || (original_offset&3u) || original_offset>=512u)return NULL;
    return slots+(original_offset>>2);
}

#include "vms_dcl_on.h"

static int upper(int c){return c>='a'&&c<='z'?c-('a'-'A'):c;}
static int white(int c){return c==' '||c=='\t';}
static int level(const char *p,unsigned len){
    /* DCL/SRC/ON.MAR LEVELTBL uses four-byte names. */
    static const struct {const char *name;int severity;} names[]={
        {"ERRO",2},{"SEVE",4},{"WARN",0},{"CONT",1}
    };
    if(len<4u)return -1;
    for(unsigned i=0;i<4u;i++){
        unsigned j=0;
        for(;j<4u&&upper(p[j])==names[i].name[j];j++){}
        if(j==4u)return names[i].severity;
    }
    return -1;
}
static int then_word(const char *p){
    return upper(p[0])=='T'&&upper(p[1])=='H'&&
           upper(p[2])=='E'&&upper(p[3])=='N'&&white(p[4]);
}
void port_dcl_on_reset(struct port_dcl_on_process *proc){
    if(!proc)return;
    proc->on_level=PORT_DCL_ON_DEFAULT_LEVEL;
    proc->on_error[0]=0;
}
void port_dcl_on_ctly_reset(struct port_dcl_on_process *proc){
    if(!proc)return;
    proc->on_control_y[0]=0;
    proc->control_enabled=0;
}
void port_dcl_on_init(struct port_dcl_on_process *proc,unsigned interactive,
                      unsigned indirect_depth){
    if(!proc)return;
    *proc=(struct port_dcl_on_process){0};
    proc->interactive=interactive?1u:0u;
    proc->indirect_depth=indirect_depth;
    port_dcl_on_reset(proc);
}
int port_dcl_on(const char *args,struct port_dcl_on_process *proc){
    if(!args||!proc)return PORT_DCL_ON_DELIMITER;
    const char *p=args;
    if(!white(*p))return PORT_DCL_ON_DELIMITER;
    while(white(*p))p++;
    const char *start=p;
    while(*p&&!white(*p))p++;
    unsigned n=(unsigned)(p-start);
    if(!n)return PORT_DCL_ON_BADLEVEL;
    int severity=level(start,n);
    if(severity<0)return PORT_DCL_ON_BADLEVEL;
    if(!white(*p))return PORT_DCL_ON_DELIMITER;
    while(white(*p))p++;
    if(!then_word(p))return PORT_DCL_ON_BADCOMMAND;
    p+=4;
    while(white(*p))p++;
    const char *command=p;
    unsigned size=0;
    while(p[size]){
        if(size>=PORT_DCL_ON_TEXT_MAX)return PORT_DCL_ON_LENGTH;
        size++;
    }
    if(!size)return PORT_DCL_ON_BADCOMMAND;
    if(proc->interactive&&!proc->indirect_depth)return PORT_DCL_ON_LEVEL;
    /* Replace only the handler of the selected class; ON CTLY is distinct. */
    char *dest=severity==1?proc->on_control_y:proc->on_error;
    if(severity==1)port_dcl_on_ctly_reset(proc);
    else port_dcl_on_reset(proc);
    for(unsigned i=0;i<size;i++)dest[i]=command[i];
    dest[size]=0;
    if(severity==1)proc->control_enabled=1;
    else proc->on_level=(unsigned)severity;
    return PORT_DCL_ON_NORMAL;
}

#include "vms_dcl_goto.h"

static unsigned upper(unsigned c){return c>='a'&&c<='z'?c-('a'-'A'):c;}
static int spaces(char c){return c==' '||c=='\t';}
static int equal(const char *a,const char *b){
    while(*a&&*b){if(upper((unsigned char)*a++)!=upper((unsigned char)*b++))return 0;}
    return !*a&&!*b;
}
static int label_copy(char dst[PORT_DCL_GOTO_MAX_LABEL+1],const char *source){
    unsigned n=0;
    if(!source)return 0;
    while(spaces(*source))source++;
    while(source[n]&&!spaces(source[n])){
        unsigned c=(unsigned char)source[n];
        if(n>=PORT_DCL_GOTO_MAX_LABEL||
           !((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
             (c>='0'&&c<='9')||c=='_'||c=='$'))return 0;
        dst[n]=(char)upper(c);n++;
    }
    if(!n)return 0;
    source+=n;
    while(spaces(*source))source++;
    if(*source)return 0;
    dst[n]=0;
    return 1;
}
void port_dcl_goto_init(struct port_dcl_goto_state *state){
    if(state)*state=(struct port_dcl_goto_state){0};
}
void port_dcl_dealgoto(struct port_dcl_goto_state *state){
    if(state){state->pending=0;state->target[0]=0;}
}
int port_dcl_goto_skipping(const struct port_dcl_goto_state *state){
    return state&&state->pending!=0;
}
int port_dcl_position(struct port_dcl_goto_state *state,const char *name,
                      const struct port_dcl_goto_ops *ops,void *ctx){
    char label[PORT_DCL_GOTO_MAX_LABEL+1];
    if(!state||!label_copy(label,name))return PORT_DCL_GOTO_BADLABEL;
    for(unsigned i=0;i<state->count;i++){
        if(equal(label,state->labels[i].name)){
            if(!ops||!ops->find_rfa||!ops->find_rfa(ctx,state->labels[i].rfa))
                return PORT_DCL_GOTO_FINDERR;
            port_dcl_dealgoto(state);
            return PORT_DCL_GOTO_NORMAL;
        }
    }
    /* The original DCL$POSITION allocates a target control block for a
       forward search. Here it is held in the process state until found. */
    for(unsigned i=0;label[i];i++)state->target[i]=label[i];
    unsigned i=0;while(label[i])i++;state->target[i]=0;
    state->pending=1;
    return PORT_DCL_GOTO_NORMAL;
}
int port_dcl_goto(struct port_dcl_goto_state *state,const char *args,
                  int random_access,const struct port_dcl_goto_ops *ops,void *ctx){
    if(!state||!args)return PORT_DCL_GOTO_BADLABEL;
    /* DCL$GOTO does not update $STATUS on a successful transfer. */
    state->no_status=1;
    if(!random_access)return PORT_DCL_GOTO_NORMAL;
    return port_dcl_position(state,args,ops,ctx);
}
int port_dcl_goto_label_record(struct port_dcl_goto_state *state,
                               const char *label,const uint8_t rfa[6]){
    char normalized[PORT_DCL_GOTO_MAX_LABEL+1];
    if(!state||!rfa||!label_copy(normalized,label))return PORT_DCL_GOTO_BADLABEL;
    unsigned i=0;
    for(;i<state->count;i++)if(equal(state->labels[i].name,normalized))break;
    if(i==state->count){
        if(state->count==PORT_DCL_GOTO_TABLE_CAP)return PORT_DCL_GOTO_NOMEM;
        state->count++;
    }
    unsigned j=0;
    for(;normalized[j];j++)state->labels[i].name[j]=normalized[j];
    state->labels[i].name[j]=0;
    for(j=0;j<6;j++)state->labels[i].rfa[j]=rfa[j];
    if(state->pending&&equal(state->target,normalized)){
        port_dcl_dealgoto(state);return PORT_DCL_GOTO_NORMAL;
    }
    return state->pending?0:PORT_DCL_GOTO_NORMAL;
}

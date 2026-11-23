#include "vms_dcl_command.h"

static unsigned upper(unsigned c){return c>='a'&&c<='z'?c-32u:c;}
static unsigned size(const char *s){unsigned i=0;while(s&&s[i])i++;return i;}
static int same(const char *a,const char *b){
    if(!a||!b)return 0;
    while(*a&&*b){if(upper((uint8_t)*a++)!=upper((uint8_t)*b++))return 0;}
    return !*a&&!*b;
}
static int copy(char *dst,unsigned cap,const char *s){
    unsigned n=size(s);
    if(n>=cap)return 0;
    for(unsigned i=0;i<=n;i++)dst[i]=s[i];
    return 1;
}
static unsigned space(unsigned c){return c==' '||c=='\t';}
static const char *skip(const char *p){while(space((uint8_t)*p))p++;return p;}
static int ident(unsigned c){return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||
                              (c>='0'&&c<='9')||c=='_'||c=='$';}
static void reset_tokens(struct port_dcl_command *s){
    s->count=0;s->param_count=0;s->qual_count=0;s->image=0;
    s->flags=0;s->no_status=0;s->verb[0]=0;s->external_image[0]=0;s->last_error=0;
}
void port_dcl_command_init(struct port_dcl_command *s){
    if(!s)return;
    uint8_t *p=(uint8_t *)(void *)s;
    for(unsigned i=0;i<sizeof *s;i++)p[i]=0;
    s->last_status=1u;
    copy(s->prompt,sizeof s->prompt,"$ ");
}
void port_dcl_cmd_start(struct port_dcl_command *s,unsigned depth,
                        unsigned control_y_level){
    if(!s)return;
    reset_tokens(s);
    s->depth=depth;s->control_y_level=control_y_level;
    copy(s->prompt,sizeof s->prompt,
         depth&&!control_y_level?"$_":"$ ");
}
static int fail(struct port_dcl_command *s,int code,
                const struct port_dcl_command_ops *ops,void *ctx){
    s->last_error=code;s->last_status=2u;
    if(ops->report)ops->report(ctx,code);
    if(ops->set_status)ops->set_status(ctx,2u);
    return code;
}
void port_dcl_cmd_disable(struct port_dcl_command *s,
                          const struct port_dcl_command_ops *ops,void *ctx){
    if(!s)return;
    if(s->ast_disabled++==0&&ops&&ops->control_ast)ops->control_ast(ctx,0);
}
void port_dcl_cmd_enable(struct port_dcl_command *s,
                         const struct port_dcl_command_ops *ops,void *ctx){
    if(!s||!s->ast_disabled)return;
    if(--s->ast_disabled==0&&ops&&ops->control_ast){
        if(!s->control_y_level&&(!s->control_y_pending||s->depth))
            ops->control_ast(ctx,1);
    }
}
int port_dcl_cmd_check_ast(struct port_dcl_command *s){
    return s&&s->control_y_pending?PORT_DCL_CMD_RESTART:PORT_DCL_CMD_OK;
}
void port_dcl_cmd_flush(struct port_dcl_command *s,unsigned terminal,
                        const struct port_dcl_command_ops *ops,void *ctx){
    if(!s||terminal||s->control_y_level)return;
    if(ops&&ops->flush)ops->flush(ctx);
    s->flushes++;
}
int port_dcl_cmd_eod(struct port_dcl_command *s,const char *tail){
    if(!s||!tail)return PORT_DCL_CMD_SYNTAX;
    if(*skip(tail))return PORT_DCL_CMD_SYNTAX;
    /* DCL$EOD sets NOSTAT on successful command completion. */
    s->no_status=1u;
    return PORT_DCL_CMD_OK;
}
static struct port_dcl_command_token *emit(struct port_dcl_command *s,
        unsigned type,unsigned index,const char *value,unsigned length){
    if(s->count>=sizeof s->tokens/sizeof s->tokens[0]||
       length>=PORT_DCL_CMD_LINE)return 0;
    struct port_dcl_command_token *t=&s->tokens[s->count++];
    for(unsigned i=0;i<length;i++)t->value[i]=value[i];
    t->value[length]=0;
    t->length=(uint16_t)length;t->type=(uint8_t)type;
    t->index=(uint8_t)index;t->separator=0;t->flags=0;
    return t;
}
/* Internal image parameter records precede command options in the result. */
static void order_internal(struct port_dcl_command *s){
    unsigned front=0;
    for(unsigned i=0;i<s->count;i++){
        if(s->tokens[i].type==PORT_DCL_CMD_RESULT_PARAMETER){
            struct port_dcl_command_token keep=s->tokens[i];
            for(unsigned j=i;j>front;j--)s->tokens[j]=s->tokens[j-1u];
            s->tokens[front++]=keep;
        }
    }
}
static int is_eol(unsigned c){return c==0||c=='!';}
/* Return a DCL value. Doubled quotes represent an embedded quote. */
static int value(const char **cursor,char *dst,unsigned cap,unsigned *len){
    const char *p=*cursor;unsigned n=0,quoted=0;
    if(*p=='"'){quoted=1;p++;}
    while(*p){
        unsigned c=(uint8_t)*p;
        if(quoted&&c=='"'){
            if(p[1]=='"'){c='"';p+=2;}
            else {p++;quoted=0;break;}
        }else if(!quoted&&(space(c)||c==','||c=='+'||c=='/'||c=='!'))break;
        else p++;
        if(n+1u>=cap)return 0;
        dst[n++]=(char)c;
    }
    if(quoted)return 0;
    if(*p&&!space((uint8_t)*p)&&*p!=','&&*p!='+'&&*p!='/'&&*p!='!')return 0;
    dst[n]=0;*cursor=p;*len=n;
    return 1;
}
static int token_name(const char **p,char *buf,unsigned cap){
    unsigned i=0;
    while(ident((uint8_t)**p)){
        if(i+1u>=cap)return 0;
        buf[i++]=(char)upper((uint8_t)*(*p)++);
    }
    buf[i]=0;
    return i>0;
}
static int option(struct port_dcl_command *s,const char **cursor){
    const char *p=*cursor;
    if(*p!='/')return PORT_DCL_CMD_SYNTAX;
    p++;
    char name[PORT_DCL_CMD_LINE];
    if(!token_name(&p,name,sizeof name))return PORT_DCL_CMD_SYNTAX;
    struct port_dcl_command_token *t=emit(s,PORT_DCL_CMD_RESULT_QUAL,
                                          s->qual_count++,name,size(name));
    if(!t)return PORT_DCL_CMD_MEMORY;
    if(*p=='='){
        char v[PORT_DCL_CMD_LINE];unsigned n;
        p++;
        if(*p=='('){
            const char *start=p;
            unsigned depth=0,quoted=0;
            do{
                unsigned c=(uint8_t)*p;
                if(!c)return PORT_DCL_CMD_SYNTAX;
                if(c=='"'){
                    if(quoted&&p[1]=='"'){p+=2;continue;}
                    quoted=!quoted;
                }else if(!quoted){
                    if(c=='(')depth++;
                    else if(c==')')depth--;
                }
                p++;
            }while(depth);
            n=(unsigned)(p-start);
            if(n>=sizeof v)return PORT_DCL_CMD_MEMORY;
            for(unsigned j=0;j<n;j++)v[j]=start[j];
            v[n]=0;
        }else if(!value(&p,v,sizeof v,&n))return PORT_DCL_CMD_SYNTAX;
        if(!emit(s,PORT_DCL_CMD_RESULT_QUAL_VALUE,
                 s->qual_count-1u,v,n))return PORT_DCL_CMD_MEMORY;
        /* Lists inside /NAME=(value,...) belong to table-driven syntax. */
    }
    *cursor=p;
    return PORT_DCL_CMD_OK;
}
static int parameters(struct port_dcl_command *s,const char *tail,
                      const struct port_dcl_command_rule *rule,
                      const struct port_dcl_command_ops *ops,void *ctx){
    const char *p=tail;
    unsigned was_delim=0;
    while(*skip(p)){
        p=skip(p);
        if(is_eol((uint8_t)*p))break;
        if(*p=='/'&&!(rule->param_flags[s->param_count<8?s->param_count:7]&
                        PORT_DCL_CMD_KEYWORDS)){
            int rc=option(s,&p);
            if(rc<0)return rc;
            continue;
        }
        if(s->param_count>=rule->max_params||
           s->param_count>=PORT_DCL_CMD_PARAM_LIMIT)return PORT_DCL_CMD_MAXPARM;
        if(*p==','||*p=='+')return PORT_DCL_CMD_DELIMITER;
        char item[PORT_DCL_CMD_LINE];unsigned len=0;
        int started_quoted=*p=='"';
        if(!value(&p,item,sizeof item,&len))return PORT_DCL_CMD_SYNTAX;
        if(!len&&!started_quoted)return PORT_DCL_CMD_DELIMITER;
        unsigned index=s->param_count;
        struct port_dcl_command_token *t=emit(s,PORT_DCL_CMD_RESULT_PARAMETER,
                                               index,item,len);
        if(!t)return PORT_DCL_CMD_MEMORY;
        if(*p==','||*p=='+'){
            unsigned sep=(uint8_t)*p++;
            if(sep==','&&!(rule->param_flags[index]&PORT_DCL_CMD_LIST))
                return PORT_DCL_CMD_NOLIST;
            if(sep=='+'&&!(rule->param_flags[index]&PORT_DCL_CMD_CONCAT))
                return PORT_DCL_CMD_NOCONCAT;
            t->separator=(uint8_t)sep;
            was_delim=sep;
            if(!*skip(p))return PORT_DCL_CMD_MISSING;
            continue;
        }
        if(!was_delim || *p!='/' )s->param_count++;
        was_delim=0;
        p=skip(p);
        if(*p=='/'&&s->param_count<PORT_DCL_CMD_PARAM_LIMIT)continue;
    }
    if(s->param_count<rule->min_params){
        if(s->batch||s->depth||s->control_y_level||!ops->read_parameter)
            return PORT_DCL_CMD_MISSING;
        for(unsigned count=0;s->param_count<rule->min_params&&count<8u;count++){
            char answer[PORT_DCL_CMD_LINE];
            const char *name=rule->param_prompt[s->param_count];
            int n=ops->read_parameter(ctx,name?name:"_",answer,sizeof answer);
            if(n<0)return PORT_DCL_CMD_INPUT;
            if(!n)return PORT_DCL_CMD_MISSING;
            if((unsigned)n>=sizeof answer)return PORT_DCL_CMD_INPUT;
            const char *q=answer;
            if(*skip(q)=='/'){
                q=skip(q);int rc=option(s,&q);
                if(rc<0)return rc;
                continue;
            }
            if(!emit(s,PORT_DCL_CMD_RESULT_PARAMETER,s->param_count,
                     answer,(unsigned)n))return PORT_DCL_CMD_MEMORY;
            s->param_count++;
        }
        if(s->param_count<rule->min_params)return PORT_DCL_CMD_MISSING;
    }
    return PORT_DCL_CMD_OK;
}
static int parse_assign(struct port_dcl_command *s,const char *name,
                        const char *rest,unsigned numeric,unsigned substring,
                        const struct port_dcl_command_ops *ops,void *ctx){
    const char *p=skip(rest);unsigned global=0;
    if(*p!='=')return PORT_DCL_CMD_SYNTAX;
    p++;
    if(*p=='='){global=1;p++;}
    p=skip(p);
    if(!ops->assign)return PORT_DCL_CMD_BADVERB;
    int rc=ops->assign(ctx,name,p,global,numeric,substring);
    if(rc<=0)return PORT_DCL_CMD_SYNTAX;
    s->last_status=1u;
    if(ops->set_status)ops->set_status(ctx,1u);
    return PORT_DCL_CMD_OK;
}
static int procedure(struct port_dcl_command *s,const char *text,
                     const struct port_dcl_command_ops *ops,void *ctx){
    const char *p=skip(text+1);
    char name[PORT_DCL_CMD_LINE];unsigned i=0;
    while(*p&&!space((uint8_t)*p)&&*p!='!'){
        if(i+1u>=sizeof name)return PORT_DCL_CMD_MEMORY;
        name[i++]=*p++;
    }
    name[i]=0;
    if(!i||!ops->open_procedure||!ops->open_procedure(ctx,name))
        return PORT_DCL_CMD_INPUT;
    s->last_status=1u;
    return PORT_DCL_CMD_SKIPPED;
}
static int process(struct port_dcl_command *s,const char *line,
                   const struct port_dcl_command_ops *ops,void *ctx){
    const char *p=skip(line);
    if(!*p||*p=='!')return PORT_DCL_CMD_SKIPPED;
    if(*p=='$'){
        p=skip(p+1);
        if(!*p||*p=='!')return PORT_DCL_CMD_SKIPPED;
    }
    if(*p=='@')return procedure(s,p,ops,ctx);
    char name[PORT_DCL_CMD_LINE];
    if(!token_name(&p,name,sizeof name)){
        if(*p=='$'){
            copy(s->external_image,sizeof s->external_image,p+1);
            return PORT_DCL_CMD_BADVERB;
        }
        return PORT_DCL_CMD_SYNTAX;
    }
    p=skip(p);
    if(*p==':'){
        p++;
        if(*p=='=')return parse_assign(s,name,p,0,0,ops,ctx);
        if(s->control_y_level)return PORT_DCL_CMD_SKIPPED;
        if(!s->depth)return PORT_DCL_CMD_SKIPPED;
        if(s->goto_pending){
            if(ops->seek_label&&ops->seek_label(ctx,name))s->goto_pending=0;
            return PORT_DCL_CMD_SKIPPED;
        }
        if(ops->label)ops->label(ctx,name);
        return PORT_DCL_CMD_SKIPPED;
    }
    if(*p=='['){
        char selection[PORT_DCL_CMD_LINE];
        unsigned k=0,j=size(name);
        if(j>=sizeof selection-1u)return PORT_DCL_CMD_MEMORY;
        for(unsigned i=0;i<j;i++)selection[k++]=name[i];
        while(*p&&*p!=']'){
            if(k+1u>=sizeof selection)return PORT_DCL_CMD_MEMORY;
            selection[k++]=*p++;
        }
        if(*p!=']'||k+1u>=sizeof selection)return PORT_DCL_CMD_SYNTAX;
        selection[k++]=*p++;selection[k]=0;
        return parse_assign(s,selection,p,1,1,ops,ctx);
    }
    if(*p=='=')return parse_assign(s,name,p,1,0,ops,ctx);
    if(s->goto_pending&&!s->control_y_level)return PORT_DCL_CMD_SKIPPED;
    char alias[PORT_DCL_CMD_LINE];
    if(ops->symbol&&ops->symbol(ctx,name,alias,sizeof alias)&&*alias){
        if(*alias=='!')return PORT_DCL_CMD_SKIPPED;
        if(s->last_alias_count++>=PORT_DCL_CMD_ALIAS_MAX)return PORT_DCL_CMD_RECURSE;
        char work[PORT_DCL_CMD_LINE];unsigned i=0;
        const char *tail=p;
        unsigned a=size(alias);
        if(a&&alias[a-1u]=='\'')a--;
        if(a>=sizeof work)return PORT_DCL_CMD_MEMORY;
        for(;i<a;i++)work[i]=alias[i];
        if(*tail&&*tail!=' '&&i+1u<sizeof work)work[i++]=' ';
        while(*tail){
            if(i+1u>=sizeof work)return PORT_DCL_CMD_MEMORY;
            work[i++]=*tail++;
        }
        work[i]=0;
        if(*work=='@')return procedure(s,work,ops,ctx);
        if(*work=='$'){
            const char *q=work+1;unsigned j=0;
            while(*q&&!space((uint8_t)*q)){
                if(j+1u>=sizeof s->external_image)return PORT_DCL_CMD_MEMORY;
                s->external_image[j++]=*q++;
            }
            s->external_image[j]=0;
            if(!copy(s->verb,sizeof s->verb,s->external_image))return PORT_DCL_CMD_MEMORY;
            struct port_dcl_command_rule foreign={
                .name=s->external_image,.image=0,.flags=PORT_DCL_CMD_FOREIGN,
                .min_params=0,.max_params=1
            };
            if(*q){
                q=skip(q);
                if(!emit(s,PORT_DCL_CMD_RESULT_PARAMETER,0,q,size(q)))
                    return PORT_DCL_CMD_MEMORY;
                s->param_count=1;
            }
            s->flags|=PORT_DCL_CMD_FOREIGN;
            s->commands++;
            if(!ops->dispatch)return PORT_DCL_CMD_BADVERB;
            return ops->dispatch(ctx,s,&foreign);
        }
        if(!copy(s->input,sizeof s->input,work))return PORT_DCL_CMD_MEMORY;
        return process(s,s->input,ops,ctx);
    }
    if(!copy(s->verb,sizeof s->verb,name))return PORT_DCL_CMD_MEMORY;
    const struct port_dcl_command_rule *rule=ops->find_verb?
        ops->find_verb(ctx,name):0;
    if(!rule)return PORT_DCL_CMD_BADVERB;
    s->image=rule->image;
    s->flags=rule->flags;
    if((rule->flags&PORT_DCL_CMD_FOREIGN)||s->run_default){
        if(!copy(s->external_image,sizeof s->external_image,rule->name))
            return PORT_DCL_CMD_MEMORY;
        const char *tail=skip(p);
        if(*tail){
            if(!emit(s,PORT_DCL_CMD_RESULT_PARAMETER,0,tail,size(tail)))
                return PORT_DCL_CMD_MEMORY;
            s->param_count=1;
        }
    }else{
        int rc=parameters(s,p,rule,ops,ctx);
        if(rc<0)return rc;
    }
    if(s->flags&PORT_DCL_CMD_INTERNAL)order_internal(s);
    if(!emit(s,PORT_DCL_CMD_RESULT_END,0,"",0))return PORT_DCL_CMD_MEMORY;
    if(s->privileged_image&&s->image&&s->image!=0x83u&&
       s->image!=0xa8u&&s->image!=0xa9u&&ops->force_exit){
        if(!ops->force_exit(ctx))return PORT_DCL_CMD_INPUT;
        s->privileged_image=0;
    }
    s->commands++;
    if(!ops->dispatch)return PORT_DCL_CMD_BADVERB;
    return ops->dispatch(ctx,s,rule);
}
int port_dcl_command_run(struct port_dcl_command *s,const char *line,
                         const struct port_dcl_command_ops *ops,void *ctx){
    if(!s||!line||!ops)return PORT_DCL_CMD_SYNTAX;
    port_dcl_cmd_start(s,s->depth,s->control_y_level);
    s->last_alias_count=0;
    if(s->control_y_pending){
        s->control_y_pending=0;
        if(s->control_command[0]){
            line=s->control_command;
            s->goto_pending=0;
        }else s->control_y_level=1;
    }
    if(s->chain_command[0]){
        line=s->chain_command;
        s->chain_command[0]=0;
    }
    if(s->hangup)return port_dcl_cmd_abort(s,ops,ctx);
    if(!copy(s->input,sizeof s->input,line))return fail(s,PORT_DCL_CMD_MEMORY,ops,ctx);
    int rc=process(s,s->input,ops,ctx);
    if(rc<0)return fail(s,rc,ops,ctx);
    if(rc==PORT_DCL_CMD_OK&&ops->set_status&&!s->no_status){
        s->last_status=1u;ops->set_status(ctx,1u);
    }
    return rc;
}
int port_dcl_cmd_abort(struct port_dcl_command *s,
                       const struct port_dcl_command_ops *ops,void *ctx){
    if(!s||!ops)return PORT_DCL_CMD_SYNTAX;
    s->hangup=0;
    const struct port_dcl_command_rule *rule=ops->find_verb?
        ops->find_verb(ctx,"LOGOUT"):0;
    if(!rule||!ops->dispatch)return fail(s,PORT_DCL_CMD_BADVERB,ops,ctx);
    port_dcl_cmd_start(s,s->depth,s->control_y_level);
    copy(s->verb,sizeof s->verb,"LOGOUT");
    s->image=rule->image;s->flags=rule->flags;
    if(!emit(s,PORT_DCL_CMD_RESULT_END,0,"",0))return PORT_DCL_CMD_MEMORY;
    return ops->dispatch(ctx,s,rule);
}
const struct port_dcl_command_token *port_dcl_cmd_parameter(
    const struct port_dcl_command *s,unsigned index){
    if(!s)return 0;
    for(unsigned i=0;i<s->count;i++)if(s->tokens[i].type==PORT_DCL_CMD_RESULT_PARAMETER&&
              s->tokens[i].index==index)return &s->tokens[i];
    return 0;
}
const struct port_dcl_command_token *port_dcl_cmd_option(
    const struct port_dcl_command *s,const char *name){
    if(!s)return 0;
    for(unsigned i=0;i<s->count;i++)if(s->tokens[i].type==PORT_DCL_CMD_RESULT_QUAL&&
            same(s->tokens[i].value,name))return &s->tokens[i];
    return 0;
}

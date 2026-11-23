#include "vms_dcl_readrec.h"

static unsigned upper(unsigned c){return c>='a'&&c<='z'?c-('a'-'A'):c;}
static int white(char c){return c==' '||c=='\t';}
static int word(unsigned c){c=upper(c);return (c>='A'&&c<='Z')||
                             (c>='0'&&c<='9')||c=='$'||c=='_';}
static int prefix(const char *s,const char *start){
    while(*start){if(upper((unsigned char)*s++)!=(unsigned char)*start++)return 0;}
    return 1;
}
static unsigned copy(char *dst,const char *src,unsigned cap){
    unsigned i=0;
    if(!cap)return 0;
    while(src[i]&&i+1u<cap){dst[i]=src[i];i++;}
    dst[i]=0;
    return i;
}
static unsigned length(const char *s){unsigned i=0;while(s[i])i++;return i;}
static void prompt(struct port_dcl_readrec *p,const char *value){
    copy(p->prompt,value,PORT_DCL_PROMPT_SIZE);
}
void port_dcl_readrec_init(struct port_dcl_readrec *p,int batch){
    if(!p)return;
    *p=(struct port_dcl_readrec){0};
    p->batch=batch?1u:0u;
    p->indirect_enabled=1u;
    prompt(p,batch?"":"$ ");
}
int port_dcl_silent_logout(struct port_dcl_readrec *p,
                           const struct port_dcl_readrec_ops *ops,void *ctx){
    if(p)p->last_status=PORT_DCL_INPUT_LOGOUT;
    if(ops&&ops->logout)ops->logout(ctx);
    return PORT_DCL_INPUT_LOGOUT;
}
int port_dcl_charerror(struct port_dcl_readrec *p,
                       const struct port_dcl_readrec_ops *ops,void *ctx,int code){
    if(!p)return PORT_DCL_INPUT_ERROR;
    p->last_status=code;
    p->line[0]=0;p->length=0;p->char_cursor=0;
    if(ops){
        if(ops->error)ops->error(ctx,code);
        if(ops->set_status)ops->set_status(ctx,code);
        if(ops->restart)ops->restart(ctx);
    }
    return code;
}
static int substitution(const struct port_dcl_readrec_ops *ops,void *ctx,
                        const char *name,char *value,unsigned cap){
    if(!ops||!name||!value||!cap)return 0;
    value[0]=0;
    if(ops->symbol&&ops->symbol(ctx,name,value,cap))return 1;
    value[0]=0;
    if(prefix(name,"F$")&&ops->lexical&&ops->lexical(ctx,name,value,cap))return 1;
    return 0;
}
/* DCL$INPUT's EXPAND pass. Scan translated text again to handle nested
   symbols. A single quote within double quotes is literal; two start an
   expansion. Comments permit only F$VERIFY lexical processing. */
int port_dcl_expand(struct port_dcl_readrec *p,
                    const struct port_dcl_readrec_ops *ops,void *ctx){
    char source[PORT_DCL_RECORD_SIZE],work[PORT_DCL_RECORD_SIZE];
    unsigned source_len;
    if(!p)return PORT_DCL_INPUT_ERROR;
    source_len=copy(source,p->line,sizeof source);
    if(source_len>=sizeof source-1u)return PORT_DCL_INPUT_SPACE;
    p->last_substitutions=0;
    for(;;){
        unsigned out=0;
        int in_double=0,in_comment=0,changed=0;
        for(unsigned i=0;source[i];){
            if(source[i]=='"'){
                if(out+1u>=sizeof work)return PORT_DCL_INPUT_SPACE;
                work[out++]=source[i++];
                if(source[i]=='"'&&in_double){
                    if(out+1u>=sizeof work)return PORT_DCL_INPUT_SPACE;
                    work[out++]=source[i++];
                }else in_double=!in_double;
                continue;
            }
            if(source[i]=='!'&&!in_double)in_comment=1;
            if(source[i]=='\''&& (!in_double||source[i+1]=='\'')){
                unsigned start=i;
                unsigned token_start=i+(unsigned)(in_double?2:1);
                unsigned j=token_start;
                char key[PORT_DCL_RECORD_SIZE];
                unsigned k=0;
                while(word((unsigned char)source[j])){
                    if(k+1u>=sizeof key)return PORT_DCL_INPUT_SPACE;
                    key[k++]=(char)upper((unsigned char)source[j++]);
                }
                /* Lexical arguments remain part of the substitution token. */
                if(k>=2u&&key[0]=='F'&&key[1]=='$'&&source[j]=='('){
                    unsigned level=0;
                    do{
                        if(k+1u>=sizeof key)return PORT_DCL_INPUT_SPACE;
                        if(source[j]=='(')level++;
                        if(source[j]==')')level--;
                        key[k++]=(char)upper((unsigned char)source[j++]);
                    }while(source[j]&&level);
                }
                key[k]=0;
                if(k&&(!in_comment||prefix(key,"F$VERIFY"))){
                    char val[PORT_DCL_RECORD_SIZE];
                    if(!substitution(ops,ctx,key,val,sizeof val)){
                        if(!in_comment)return PORT_DCL_INPUT_SYMBOL;
                    }else{
                        if(p->last_substitutions++>=PORT_DCL_SUBSTITUTION_LIMIT)
                            return PORT_DCL_INPUT_RECURSE;
                        unsigned v=length(val);
                        if(out+v+1u>=sizeof work)return PORT_DCL_INPUT_SPACE;
                        for(unsigned n=0;n<v;n++)work[out++]=val[n];
                        i=j;
                        if(source[i]=='\'')i++;
                        changed=1;
                        continue;
                    }
                }
                /* A leading quote without a valid name remains literal. */
                if(out+1u>=sizeof work)return PORT_DCL_INPUT_SPACE;
                work[out++]=source[start];
                i=start+1u;
                continue;
            }
            if(out+1u>=sizeof work)return PORT_DCL_INPUT_SPACE;
            work[out++]=source[i++];
        }
        work[out]=0;
        copy(source,work,sizeof source);
        if(!changed){p->length=copy(p->line,source,sizeof p->line);return PORT_DCL_INPUT_OK;}
    }
}
/* DCL$SPECIAL dispatch and the two lexical prefix forms. */
int port_dcl_special(struct port_dcl_readrec *p,const char *token,
                     const struct port_dcl_readrec_ops *ops,void *ctx,
                     char *replacement,unsigned cap){
    if(!p||!token||!replacement||!cap)return PORT_DCL_INPUT_ERROR;
    replacement[0]=0;
    if(token[0]=='&'){
        char key[PORT_DCL_RECORD_SIZE];unsigned i=0;
        for(const char *name=token+1;*name;name++){
            if(!word((unsigned char)*name)||i+1u>=sizeof key)return PORT_DCL_INPUT_SYMBOL;
            key[i++]=(char)upper((unsigned char)*name);
        }
        if(!i)return PORT_DCL_INPUT_OK;
        key[i]=0;
        if(!substitution(ops,ctx,key,replacement,cap))return PORT_DCL_INPUT_SYMBOL;
        return PORT_DCL_INPUT_OK;
    }
    if(token[0]=='@'){
        if(p->goto_search||!p->indirect_enabled)return PORT_DCL_INPUT_OK;
        if(!token[1]||p->depth>=PORT_DCL_INDIRECT_LEVELS||!ops||!ops->push)
            return PORT_DCL_INPUT_BADFILE;
        p->indirect_enabled=0;
        int pushed=ops->push(ctx,p->depth+1u,token+1);
        p->indirect_enabled=1;
        if(!pushed)return PORT_DCL_INPUT_BADFILE;
        p->depth++;
        p->last_was_indirect=1;
        return PORT_DCL_INPUT_INDIRECT;
    }
    copy(replacement,token,cap);
    return PORT_DCL_INPUT_OK;
}
static int all_comment(const char *line){
    while(white(*line))line++;
    if(*line=='$'){
        line++;
        while(white(*line))line++;
    }
    return !*line||*line=='!';
}
static int has_label(const char *line){
    for(;*line;line++)if(*line==':')return 1;
    return 0;
}
/* DCL$INPUT executes before syntax parsing. The caller supplies RMS $GET,
   $CANCEL, DCL$UNSTACK, symbol and output services. */
int port_dcl_input(struct port_dcl_readrec *p,
                   const struct port_dcl_readrec_ops *ops,void *ctx){
    if(!p||!ops||!ops->get)return PORT_DCL_INPUT_ERROR;
    for(unsigned reads=0;reads<2048u;reads++){
        if(p->auto_logout&&!p->inquire&&(p->control_y_level||!p->depth))
            return port_dcl_silent_logout(p,ops,ctx);
        unsigned selected=(p->inquire||p->control_y_level)?0u:p->depth;
        char incoming[PORT_DCL_RECORD_SIZE];
        unsigned n=0;
        int result;
        if(selected){p->prompt[0]=0;}
        if(ops->disable_control_y)ops->disable_control_y(ctx);
        result=ops->get(ctx,selected,p->prompt,incoming,sizeof incoming);
        if(ops->enable_control_y)ops->enable_control_y(ctx);
        p->last_retry_count=0;
        if(result==PORT_DCL_INPUT_RSA){
            /* $GET with RMS$_RSA: retry, then $CANCEL the input channel. */
            do{
                p->last_retry_count++;
                if(ops->disable_control_y)ops->disable_control_y(ctx);
                result=ops->get(ctx,selected,p->prompt,incoming,sizeof incoming);
                if(ops->enable_control_y)ops->enable_control_y(ctx);
            }while(result==PORT_DCL_INPUT_RSA&&p->last_retry_count<1000u);
            if(result==PORT_DCL_INPUT_RSA){
                if(ops->cancel)ops->cancel(ctx,selected);
                continue;
            }
        }
        if(result==PORT_DCL_INPUT_EOF){
            if(!p->inquire&&!p->control_y_level&&p->depth){
                if(ops->pop)ops->pop(ctx,p->depth);
                p->depth--;
                if(!p->depth)prompt(p,"$ ");
                continue;
            }
            if(!p->inquire&&!p->control_y_level&&p->goto_search){
                p->goto_search=0;
                return port_dcl_charerror(p,ops,ctx,PORT_DCL_INPUT_BADFILE);
            }
            if(p->batch||p->eof_logout)return port_dcl_silent_logout(p,ops,ctx);
            continue;
        }
        if(result<0){
            if(!p->inquire&&!p->control_y_level&&p->depth){
                if(ops->pop)ops->pop(ctx,p->depth);
                p->depth--;
            }
            return port_dcl_charerror(p,ops,ctx,result);
        }
        if((unsigned)result>=sizeof incoming)
            return port_dcl_charerror(p,ops,ctx,PORT_DCL_INPUT_SPACE);
        n=(unsigned)result;
        incoming[n]=0;
        if(p->goto_search&&!p->control_y_level&&!has_label(incoming))continue;
        p->length=copy(p->line,incoming,sizeof p->line);
        if(p->length!=n)return port_dcl_charerror(p,ops,ctx,PORT_DCL_INPUT_SPACE);
        int needs_expand=0;
        for(unsigned i=0;i<p->length;i++)if(p->line[i]=='\''){needs_expand=1;break;}
        if(needs_expand){
            int rc=port_dcl_expand(p,ops,ctx);
            if(rc!=PORT_DCL_INPUT_OK)return port_dcl_charerror(p,ops,ctx,rc);
        }
        if(p->verify&&!p->control_y_level&&ops->verify)ops->verify(ctx,p->line);
        if(!p->control_y_pending&&all_comment(p->line))continue;
        if(p->trailing_space){
            if(p->length+1u>=sizeof p->line)
                return port_dcl_charerror(p,ops,ctx,PORT_DCL_INPUT_SPACE);
            for(unsigned j=p->length+1u;j;j--)p->line[j]=p->line[j-1u];
            p->line[0]=' ';
            p->length++;
            p->trailing_space=0;
        }
        p->char_cursor=0;
        prompt(p,"_ ");
        p->last_status=PORT_DCL_INPUT_OK;
        return PORT_DCL_INPUT_OK;
    }
    return port_dcl_charerror(p,ops,ctx,PORT_DCL_INPUT_RECURSE);
}

#include "vms_dcl_inquire.h"

static int upper(int c){return c>='a'&&c<='z'?c-('a'-'A'):c;}
static int alpha(int c){c=upper(c);return c>='A'&&c<='Z';}
static int symchar(int c){return alpha(c)||(c>='0'&&c<='9')||c=='_'||c=='$';}
static int white(int c){return c==' '||c=='\t';}
static unsigned length(const char *s){unsigned n=0;while(s[n])n++;return n;}
static int same(const char *a,unsigned count,const char *b){
    unsigned i=0;
    while(i<count&&b[i]&&upper(a[i])==upper(b[i]))i++;
    return count>0u&&i==count;
}
static int append(char *dst,unsigned cap,unsigned *pos,char c){
    if(*pos+1u>=cap)return 0;
    dst[(*pos)++]=c;
    dst[*pos]=0;
    return 1;
}
/* The source's DCL$COMPRESS handles quoted strings and paired quotes. */
static int compress(const char *src,char *dst,unsigned cap){
    unsigned j=0;int quoted=0;
    dst[0]=0;
    for(unsigned i=0;src[i];i++){
        if(src[i]=='"'){
            if(quoted && src[i+1]=='"'){
                if(!append(dst,cap,&j,'"'))return 0;
                i++;
            } else quoted=!quoted;
        } else if(!append(dst,cap,&j,src[i]))return 0;
    }
    return !quoted;
}
int port_dcl_inquire_parse(const char *args,struct port_dcl_inquire_request *out){
    if(!args||!out)return PORT_DCL_INQUIRE_SYNTAX;
    *out=(struct port_dcl_inquire_request){0};
    out->punctuation=1;
    const char *p=args;
    while(white(*p))p++;
    while(*p=='/'){
        const char *first=++p;
        while(alpha(*p))p++;
        unsigned n=(unsigned)(p-first);
        if(same(first,n,"GLOBAL"))out->global=1;
        else if(same(first,n,"LOCAL"))out->global=0;
        else if(same(first,n,"PUNCTUATION"))out->punctuation=1;
        else if(same(first,n,"NOPUNCTUATION"))out->punctuation=0;
        else return PORT_DCL_INQUIRE_SYNTAX;
        if(*p && !white(*p)&&*p!='/')return PORT_DCL_INQUIRE_SYNTAX;
        while(white(*p))p++;
    }
    if(!alpha(*p))return PORT_DCL_INQUIRE_SYNTAX;
    unsigned s=0;
    while(symchar(*p)){
        if(!append(out->symbol,sizeof out->symbol,&s,(char)upper(*p++)))
            return PORT_DCL_INQUIRE_SPACE;
    }
    if(*p && !white(*p)&&*p!='/')return PORT_DCL_INQUIRE_SYNTAX;
    while(white(*p))p++;
    while(*p=='/'){
        const char *first=++p;
        while(alpha(*p))p++;
        unsigned n=(unsigned)(p-first);
        if(same(first,n,"GLOBAL"))out->global=1;
        else if(same(first,n,"LOCAL"))out->global=0;
        else if(same(first,n,"PUNCTUATION"))out->punctuation=1;
        else if(same(first,n,"NOPUNCTUATION"))out->punctuation=0;
        else return PORT_DCL_INQUIRE_SYNTAX;
        if(*p && !white(*p)&&*p!='/')return PORT_DCL_INQUIRE_SYNTAX;
        while(white(*p))p++;
    }
    /* Empty prompt uses the symbol, as in the original DCL$MOVTOKN branch. */
    const char *text=*p?p:out->symbol;
    if(!compress(text,out->prompt,sizeof out->prompt))return PORT_DCL_INQUIRE_SYNTAX;
    unsigned n=length(out->prompt);
    if(out->punctuation){
        if(!append(out->prompt,sizeof out->prompt,&n,':')||
           !append(out->prompt,sizeof out->prompt,&n,' '))return PORT_DCL_INQUIRE_SPACE;
    }
    return PORT_DCL_INQUIRE_OK;
}
int port_dcl_inquire(const char *args,const struct port_dcl_inquire_ops *ops,void *ctx){
    if(!ops||!ops->input||!ops->store)return PORT_DCL_INQUIRE_SYNTAX;
    struct port_dcl_inquire_request request;
    int rc=port_dcl_inquire_parse(args,&request);
    if(rc!=PORT_DCL_INQUIRE_OK)return rc;
    char answer[PORT_DCL_INQUIRE_TEXT_MAX+1];
    if(ops->disable_indirect)ops->disable_indirect(ctx);
    int len=ops->input(ctx,request.prompt,answer,sizeof answer);
    if(len<0)return PORT_DCL_INQUIRE_IO;
    if((unsigned)len>=sizeof answer)return PORT_DCL_INQUIRE_SPACE;
    answer[len]=0;
    char value[PORT_DCL_INQUIRE_TEXT_MAX+1];
    if(!compress(answer,value,sizeof value))return PORT_DCL_INQUIRE_SYNTAX;
    if(ops->disable_control_ast)ops->disable_control_ast(ctx);
    return ops->store(ctx,request.symbol,value,request.global)?
           PORT_DCL_INQUIRE_OK:PORT_DCL_INQUIRE_SPACE;
}

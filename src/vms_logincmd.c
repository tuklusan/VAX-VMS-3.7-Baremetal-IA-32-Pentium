#include "vms_logincmd.h"

static int space(char c) {return c==' ' || c=='\t';}
static char upper(char c){return (c>='a' && c<='z')?(char)(c-'a'+'A'):c;}
static unsigned length(const char *s){unsigned n=0;while(s[n])n++;return n;}
static int prefix(const char *actual,const char *word){
    unsigned i=0;
    while(actual[i] && word[i]){
        if(upper(actual[i])!=word[i])return 0;
        i++;
    }
    return i!=0 && !actual[i];
}
static int token(const char **cursor,char *out,unsigned capacity){
    const char *p=*cursor;
    unsigned n=0;
    if(*p=='"'){
        p++;
        while(*p && *p!='"') {
            if(*p=='"' && p[1]=='"')p++;
            if(n+1>=capacity)return VMS_LOGIN_CMD_TOOLONG;
            out[n++]=upper(*p++);
        }
        if(*p!='"')return VMS_LOGIN_CMD_SYNTAX;
        p++;
    } else {
        while(*p && !space(*p) && *p!='/'){
            if(n+1>=capacity)return VMS_LOGIN_CMD_TOOLONG;
            out[n++]=upper(*p++);
        }
    }
    out[n]=0;
    *cursor=p;
    return n?VMS_LOGIN_CMD_OK:VMS_LOGIN_CMD_SYNTAX;
}
int vms_login_parse(const char *line,struct vms_login_command *result){
    if(!line||!result)return VMS_LOGIN_CMD_SYNTAX;
    *result=(struct vms_login_command){0};
    result->command_enabled=1;
    const char *p=line;
    while(space(*p))p++;
    char word[VMS_LOGIN_VALUE_MAX];
    int rc=token(&p,word,sizeof word);
    if(rc<0 || !prefix(word,"LOGIN") || !*word)return VMS_LOGIN_CMD_SYNTAX;
    unsigned seen=0;
    for(;;){
        while(space(*p))p++;
        if(!*p)break;
        if(*p!='/'){
            if(result->has_username)return VMS_LOGIN_CMD_SYNTAX;
            rc=token(&p,result->username,sizeof result->username);
            if(rc<0)return rc;
            result->has_username=1;
            if(*p && !space(*p) && *p!='/')return VMS_LOGIN_CMD_SYNTAX;
            continue;
        }
        p++;
        unsigned n=0;
        while((*p>='A'&&*p<='Z')||(*p>='a'&&*p<='z')){
            if(n+1>=sizeof word)return VMS_LOGIN_CMD_TOOLONG;
            word[n++]=upper(*p++);
        }
        word[n]=0;
        if(!n)return VMS_LOGIN_CMD_SYNTAX;
        unsigned kind=0;
        if(prefix(word,"DISK"))kind|=1;
        if(prefix(word,"CLI"))kind|=2;
        if(prefix(word,"COMMAND"))kind|=4;
        if(prefix(word,"NOCOMMAND"))kind|=8;
        if(!kind)return VMS_LOGIN_CMD_SYNTAX;
        if(kind&(kind-1u))return VMS_LOGIN_CMD_AMBIGUOUS;
        if((seen&kind))return VMS_LOGIN_CMD_SYNTAX;
        seen|=kind;
        if(kind==8){
            if(*p=='=')return VMS_LOGIN_CMD_SYNTAX;
            result->command_enabled=0;
            continue;
        }
        char *dst=kind==1?result->disk:kind==2?result->cli:result->command;
        if(*p=='='){
            p++;
            rc=token(&p,dst,VMS_LOGIN_VALUE_MAX);
            if(rc<0)return rc;
            if(kind==4)result->has_command_value=1;
        } else if(kind!=4) return VMS_LOGIN_CMD_SYNTAX;
        if(kind==1)result->has_disk=1;
        if(kind==2)result->has_cli=1;
        if(kind==4)result->command_enabled=1;
        if(*p && *p!='/' && !space(*p))return VMS_LOGIN_CMD_SYNTAX;
    }
    if(length(result->username)>VMS_LOGIN_USER_MAX)return VMS_LOGIN_CMD_TOOLONG;
    return result->has_username?VMS_LOGIN_CMD_OK:VMS_LOGIN_CMD_PROMPT;
}

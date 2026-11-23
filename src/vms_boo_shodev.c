#include "vms_boo_shodev.h"
#include <stddef.h>
struct output {char text[128];unsigned size;};
static void ch(struct output *o,char c){if(o->size<sizeof(o->text))o->text[o->size++]=c;}
static void str(struct output *o,const char *s,unsigned limit){
    if(!s)return;
    for(unsigned i=0;i<limit&&s[i];i++)ch(o,s[i]);
}
static void hex(struct output *o,uint32_t n){
    static const char tab[]="0123456789ABCDEF";
    for(unsigned s=28;;s-=4){ch(o,tab[(n>>s)&15]);if(!s)break;}
}
static void dec(struct output *o,uint32_t n){
    char b[10];unsigned i=0;
    do{b[i++]=(char)('0'+n%10);n/=10;}while(n&&i<sizeof b);
    while(i)ch(o,b[--i]);
}
static uint16_t namelen(const char *p){
    uint16_t n=0;if(!p)return 0;while(n<255u && p[n])n++;return n;
}
static int matching(const char *a,const char *b,uint16_t len){
    if(!len)return 1;
    if(!a||!b)return 0;
    for(unsigned i=0;i<len;i++)if(!a[i]||a[i]!=b[i])return 0;
    return 1;
}
static uint32_t send(struct port_boo_shodev *s,const struct output *o){
    if(!s->write_line)return PORT_SS_BADPARAM;
    return s->write_line(s->context,o->text,(uint16_t)o->size);
}
uint32_t port_boo_shodev(struct port_boo_shodev *s,const char *token,
                          uint16_t token_size,uint8_t driver_mode){
    if(!s||!s->change_to_exec||!s->write_line||(token_size&&!token))return PORT_SS_BADPARAM;
    uint32_t st=s->change_to_exec(s->context);
    if(!(st&1u)){
        if(s->signal)s->signal(s->context,st);
        return PORT_SS_NORMAL; /* Original parser reports success after signaling. */
    }
    struct output out={{0},0};
    str(&out,"\r\n__Driver_____Start____End___",32);
    if(!driver_mode)str(&out,"_Dev___DDB______CRB______IDB____Unit___UCB____",48);
    st=send(s,&out);if(!(st&1u))return st;
    for(const struct port_boo_driver *d=s->drivers;d;d=d->next){
        if(!matching(token,d->name,token_size))continue;
        out.size=0;
        str(&out,d->name,10);ch(&out,' ');hex(&out,d->address);ch(&out,' ');
        hex(&out,d->address+d->size);
        st=send(s,&out);if(!(st&1u))return st;
        if(driver_mode)continue;
        for(const struct port_boo_devblock *p=d->devices;p;p=p->next){
            if(!matching(d->name,p->driver_name,namelen(d->name)) ||
               !p->driver_name || p->driver_name[namelen(d->name)])continue;
            out.size=0;
            str(&out,p->name,10);ch(&out,' ');hex(&out,p->address);ch(&out,' ');
            hex(&out,p->crb);ch(&out,' ');hex(&out,p->idb);
            st=send(s,&out);if(!(st&1u))return st;
            for(const struct port_boo_unit *u=p->units;u;u=u->next){
                out.size=0;dec(&out,u->number);ch(&out,' ');hex(&out,u->address);
                st=send(s,&out);if(!(st&1u))return st;
            }
        }
    }
    return PORT_SS_NORMAL;
}

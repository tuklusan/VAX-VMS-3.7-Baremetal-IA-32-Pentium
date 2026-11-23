#include "vms_sys85.h"
#include "vms_rtl85.h"

/* NLDRIVER.MAR: NULL device, with no physical input or output transfer. */
const struct port_sys85_null_dpt port_nl_driver_dpt={
    "NLDRIVER",116,
    {PORT_SYS85_READLBLK,PORT_SYS85_READVBLK,PORT_SYS85_WRITEOF,
     PORT_SYS85_WRITELBLK,PORT_SYS85_WRITEVBLK},5
};
uint32_t port_nl_driver_io(uint16_t func,void *context,port_sys85_finish_io_fn finish){
    uint32_t status;
    switch(func & 63u){
    case PORT_SYS85_READLBLK: case PORT_SYS85_READVBLK:
        status=PORT_SYS85_ENDOFFILE;
        break;
    case PORT_SYS85_WRITEOF: case PORT_SYS85_WRITELBLK:
    case PORT_SYS85_WRITEVBLK:
        status=PORT_SYS85_NORMAL;
        break;
    default: return PORT_SYS85_BADFUNC;
    }
    return finish ? finish(context,status) : status;
}

/* RMSRESET.MAR: free lists and image I/O area are per process. */
uint32_t port_rm_reset(struct port_sys85_rms *r){
    if(!r)return PORT_SYS85_BADFUNC;
    r->free_space.next=r->free_space.prev=&r->free_space;
    r->free_pages.next=r->free_pages.prev=&r->free_pages;
    r->segment_address=0;
    r->segment_length=0;
    r->ifab_table_link=0;
    r->irab_table_link=0;
    if(!r->active_ifabs)return PORT_SYS85_NORMAL;
    r->active_ifabs=0;
    if(r->handle_table){
        for(size_t i=0;i<r->handle_table_count;i++)r->handle_table[i]=0;
    }
    return PORT_SYS85_MUSTCLOSEFL;
}

/* MTFDT.MAR: software lock and volume-owner access checks. */
uint32_t port_mt_check_access(const struct port_sys85_tape *t,void *context,
    port_sys85_check_write_fn check,port_sys85_abort_io_fn abort_request){
    if(!t)return PORT_SYS85_BADFUNC;
    uint32_t status;
    if(t->device_characteristics&t->software_write_lock_mask){
        status=PORT_SYS85_WRITLCK;
    } else if(check){
        status=check(context,t->volume_protection,t->owner_uic);
    } else return PORT_SYS85_BADFUNC;
    if(status&1u)return status;
    return abort_request ? abort_request(context,status) : status;
}

/* CVTFILNAM.MAR: 9.3;version Files-11 RAD50 word encoding. */
static int rad50_char(uint8_t c){
    if(c>='A' && c<='Z')return (int)c-'A'+1;
    if(c>='a' && c<='z')return (int)c-'a'+1;
    if(c>='0' && c<='9')return (int)c-'0'+30;
    return -1;
}
static uint16_t rad50_word(const uint8_t *c){
    return (uint16_t)(((uint32_t)c[0]*40u+(uint32_t)c[1])*40u+c[2]);
}
uint32_t port_fil_cvtfilnam(const uint8_t *text,size_t n,struct port_sys85_fnb *out){
    uint8_t chars[12]={0};
    size_t p=0,field=0;
    uint32_t version=0;
    int version_specified=0;
    if(!out || (!text&&n))return PORT_SYS85_BADFILENAME;
    /* Preflight the full name, so invalid input does not alter output. */
    for(size_t i=0;i<n;i++){
        uint8_t c=text[i];
        if(field==0 && c=='.') {field=1;p=0;continue;}
        if((field==0||field==1) && c==';'){
            field=2;
            version_specified=1;
            if(!port_sys_cvt_dtb((uint32_t)(n-i-1),text+i+1,&version))
                return PORT_SYS85_BADFILENAME;
            break;
        }
        if(field==1 && c=='.'){
            field=2;
            version_specified=1;
            if(!port_sys_cvt_dtb((uint32_t)(n-i-1),text+i+1,&version))
                return PORT_SYS85_BADFILENAME;
            break;
        }
        if(p>=(field==0?9u:3u))return PORT_SYS85_BADFILENAME;
        int val=rad50_char(c);
        if(val<0)return PORT_SYS85_BADFILENAME;
        chars[(field==0?0u:9u)+p++]=(uint8_t)val;
    }
    struct port_sys85_fnb next=*out;
    if(chars[0]){
        next.filename[0]=rad50_word(chars);
        next.filename[1]=rad50_word(chars+3);
        next.filename[2]=rad50_word(chars+6);
    }
    if(chars[9]) next.filetype=rad50_word(chars+9);
    if(version_specified) next.version=(uint16_t)version;
    *out=next;
    return PORT_SYS85_NORMAL;
}

/* FORKCNTRL.MAR: per-IPL FIFO queues, with actual soft-interrupt request. */
void port_exe_fork_init(struct port_sys85_fork_system *s,
    void (*request)(void *,uint8_t),void *arg){
    if(!s)return;
    for(unsigned i=0;i<12;i++)s->head[i]=s->tail[i]=0;
    s->pending_levels=0;
    s->request_softint=request;
    s->irq_arg=arg;
}
static int fork_level(uint8_t ipl){
    return ipl==6 || ipl==8 || ipl==9 || ipl==10 || ipl==11;
}
uint32_t port_exe_fork(struct port_sys85_fork_system *s,struct port_sys85_fork *f,
                        uint32_t r3,uint32_t r4){
    if(!s || !f || !f->entry || !fork_level(f->fork_ipl) || !s->request_softint ||
        f->pending)return PORT_SYS85_BADFUNC;
    uint8_t p=f->fork_ipl;
    f->saved_r3=r3;
    f->saved_r4=r4;
    f->next=0;
    f->pending=1;
    if(s->tail[p])s->tail[p]->next=f;
    else s->head[p]=f;
    s->tail[p]=f;
    s->pending_levels|=1u<<p;
    s->request_softint(s->irq_arg,p);
    return PORT_SYS85_NORMAL;
}
uint32_t port_exe_iofork(struct port_sys85_fork_system *s,
    struct port_sys85_fork *f,uint32_t r3,uint32_t r4,
    uint16_t *device_status,uint16_t timeout_mask){
    if(!device_status)return PORT_SYS85_BADFUNC;
    /* The UCB timeout bit is cleared on the path into EXE$FORK. */
    *device_status=(uint16_t)(*device_status & (uint16_t)~timeout_mask);
    return port_exe_fork(s,f,r3,r4);
}
uint32_t port_exe_fork_dispatch(struct port_sys85_fork_system *s,uint8_t ipl){
    if(!s || !fork_level(ipl))return PORT_SYS85_BADFUNC;
    while(s->head[ipl]){
        struct port_sys85_fork *f=s->head[ipl];
        s->head[ipl]=f->next;
        if(!s->head[ipl])s->tail[ipl]=0;
        f->next=0;
        f->pending=0;
        f->entry(f,f->arg);
    }
    s->pending_levels&=~(1u<<ipl);
    return PORT_SYS85_NORMAL;
}

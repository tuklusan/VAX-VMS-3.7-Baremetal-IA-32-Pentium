#include "vms_for_m112.h"
/* Source: FORIOBEG.B32 original statement ordinals 0..20.
 * Exact STMT_ARG and ERR_ADR_IDX arrays below derived from original source.
 * VAX calling frame, RMS file record services, and error-handler unwind
 * cannot be implemented solely by selecting an I/O statement ordinal.
 */
static const uint8_t argument_flags[21]={
    0,33,33,0,0,35,35,2,2,32,32,53,53,33,41,0,8,49,49,33,33};
static const uint8_t error_argument_index[21]={
    0,3,3,2,2,4,4,3,3,2,2,4,4,3,6,2,5,3,3,3,3};
/* Explicit native bitset for each original 'forbidden unit attributes'
 * entry: READ_ONLY=1, DIRECT=2, FORMATTED=4, UNFORMATTED=8,
 * SEQUENTIAL=16, KEYED=32. Original FORIOBEG lines 255..277. */
static const uint8_t forbidden_modes[21]={
    0,11,10,7,6,57,56,53,52,43,42,0,0,11,26,7,22,0,0,11,10};
uint32_t port112_statement_argument_flags(uint32_t kind){
    return kind<=20u?argument_flags[kind]:0u;
}
uint32_t port112_statement_error_arg_index(uint32_t kind){
    return kind<=20u?error_argument_index[kind]:0u;
}
uint32_t port112_statement_forbidden_modes(uint32_t kind){
    return kind<=20u?forbidden_modes[kind]:0u;
}
int port112_unit_init(struct port112_unit *u,uint32_t number){
    if(!u || number>99999u)return PORT112_BADARG;
    struct port112_unit zero={0};*u=zero;
    u->number=number;u->record_number=1u;
    return PORT112_OK;
}
static int done(const struct port112_backend *b,enum port112_action action,
                const struct port112_unit *u,uint32_t flags,uint32_t record){
    if(!b||!b->operate)return PORT112_NO_BACKEND;
    return b->operate(b->context,action,u->number,flags,record)==PORT112_OK ?
           PORT112_OK : PORT112_IO_ERROR;
}
/* Does not issue synthetic RMS I/O: the callback must perform it or fail. */
int port112_io_begin(struct port112_unit *u,const struct port112_backend *b,
                     uint32_t statement){
    uint32_t kind=statement&255u;
    if(!u || kind<1u||kind>20u || (statement & ~0x1ffu))return PORT112_BADARG;
    if(u->in_statement)return PORT112_BUSY;
    /* Object-time format works only on formatted I/O (ARG_FMT). */
    if((statement&PORT112_OBJ_FMT) && !(argument_flags[kind]&1u))
        return PORT112_MODE;
    const int internal= kind==11u||kind==12u||kind==17u||kind==18u;
    uint32_t attrs=u->attributes;
    if(!internal){
        if(u->opened){
            if(!attrs || (attrs&forbidden_modes[kind]))return PORT112_MODE;
        }else{
            if(!b||!b->operate)return PORT112_NO_BACKEND;
            /* Preserve a native DEFINE FILE specification across default OPEN;
             * otherwise select attributes from this first I/O statement. */
            if(!attrs){
                if(kind==5u||kind==6u||kind==7u||kind==8u)attrs=PORT112_DIRECT;
                else if(kind==14u||kind==16u)attrs=PORT112_KEYED;
                else attrs=PORT112_SEQUENTIAL;
                attrs|=(kind==3u||kind==4u||kind==7u||kind==8u||kind==15u||kind==16u)?
                          PORT112_UNFORMATTED:PORT112_FORMATTED;
            }
            if(attrs&forbidden_modes[kind])return PORT112_MODE;
            if(done(b,PORT112_ACT_OPEN,u,statement,u->record_number)!=PORT112_OK)
                return PORT112_IO_ERROR;
            u->opened=1u;u->attributes=attrs;
        }
    }
    int r=done(b,PORT112_ACT_BEGIN,u,statement,u->record_number);
    if(r!=PORT112_OK)return r;
    u->statement_kind=statement;
    u->current_args=argument_flags[kind] | (statement&PORT112_OBJ_FMT?0x40u:0u);
    u->err_arg_index=error_argument_index[kind];
    u->in_statement=1u;u->last_iostat=0u;
    return PORT112_OK;
}
int port112_io_end(struct port112_unit *u,const struct port112_backend *b){
    if(!u)return PORT112_BADARG;
    if(!u->in_statement)return PORT112_STATE;
    int r=done(b,PORT112_ACT_END,u,u->statement_kind,u->record_number);
    if(r!=PORT112_OK)return r;
    u->in_statement=0u;u->current_args=0u;u->statement_kind=0u;
    return PORT112_OK;
}
int port112_associated(struct port112_unit *u){
    if(!u)return PORT112_BADARG;
    if(!u->associated_variable)return PORT112_OK;
    if(u->associated_bytes!=2u&&u->associated_bytes!=4u)return PORT112_BADARG;
    uint32_t value=u->record_number;
    uint8_t *p=(uint8_t *)u->associated_variable;
    for(unsigned i=0;i<u->associated_bytes;i++)p[i]=(uint8_t)(value>>(i*8u));
    return PORT112_OK;
}
int port112_backspace(struct port112_unit *u,const struct port112_backend *b){
    if(!u)return PORT112_BADARG;
    if(u->in_statement)return PORT112_BUSY;
    if(!u->opened)return PORT112_OK;
    if(u->attributes&(PORT112_DIRECT|PORT112_KEYED|PORT112_NOTSEQORG|PORT112_APPEND))
        return PORT112_MODE;
    int r=done(b,PORT112_ACT_BACKSPACE,u,0u,u->record_number);
    if(r!=PORT112_OK)return r;
    if(u->record_number>1u)u->record_number--;
    return port112_associated(u);
}
int port112_close(struct port112_unit *u,const struct port112_backend *b){
    if(!u)return PORT112_BADARG;
    if(u->in_statement)return PORT112_BUSY;
    if(!u->opened)return PORT112_OK;
    int r=done(b,PORT112_ACT_CLOSE,u,0u,u->record_number);
    if(r!=PORT112_OK)return r;
    u->opened=0u;u->attributes=0u;u->record_number=1u;
    return PORT112_OK;
}
int port112_define(struct port112_unit *u,uint32_t max_records,
                   uint32_t record_words,void *associated,unsigned associated_bytes){
    if(!u||u->in_statement)return PORT112_BADARG;
    if(u->opened || (u->attributes&PORT112_DIRECT))return PORT112_STATE;
    if(!max_records||!record_words||record_words>32767u ||
       (associated && associated_bytes!=2u && associated_bytes!=4u) ||
       (!associated && associated_bytes))return PORT112_RANGE;
    u->maximum_records=max_records;
    u->record_bytes=record_words*2u;
    u->attributes=PORT112_DIRECT|PORT112_UNFORMATTED;
    u->associated_variable=associated;
    u->associated_bytes=associated_bytes;
    return port112_associated(u);
}
int port112_delete(struct port112_unit *u,const struct port112_backend *b,int direct){
    if(!u)return PORT112_BADARG;
    if(u->in_statement)return PORT112_BUSY;
    if(!u->opened || (u->attributes&PORT112_READ_ONLY))return PORT112_MODE;
    if(direct && !(u->attributes&PORT112_DIRECT))return PORT112_MODE;
    if(!direct && (u->attributes&PORT112_SEQUENTIAL))return PORT112_MODE;
    if(done(b,PORT112_ACT_DELETE,u,(uint32_t)(direct!=0),u->record_number)!=PORT112_OK)
        return PORT112_IO_ERROR;
    return PORT112_OK;
}
int port112_endfile(struct port112_unit *u,const struct port112_backend *b){
    if(!u)return PORT112_BADARG;
    if(u->in_statement)return PORT112_BUSY;
    if(!u->opened)return PORT112_OK;
    if(u->attributes&(PORT112_DIRECT|PORT112_KEYED|PORT112_READ_ONLY))
        return PORT112_MODE;
    if(done(b,PORT112_ACT_ENDFILE,u,0u,u->record_number)!=PORT112_OK)
        return PORT112_IO_ERROR;
    return PORT112_OK;
}
int port112_rewind(struct port112_unit *u,const struct port112_backend *b){
    if(!u)return PORT112_BADARG;
    if(u->in_statement)return PORT112_BUSY;
    if(!u->opened)return PORT112_OK;
    if(u->attributes&(PORT112_DIRECT|PORT112_KEYED|PORT112_NOTSEQORG))
        return PORT112_MODE;
    int r=done(b,PORT112_ACT_REWIND,u,0u,u->record_number);
    if(r!=PORT112_OK)return r;
    u->record_number=1u;u->attributes&=~PORT112_APPEND;
    return port112_associated(u);
}
int port112_unlock(struct port112_unit *u,const struct port112_backend *b){
    if(!u)return PORT112_BADARG;
    if(u->in_statement)return PORT112_BUSY;
    if(!u->opened || !(u->attributes&(PORT112_KEYED|PORT112_NOTSEQORG)))
        return PORT112_MODE;
    return done(b,PORT112_ACT_UNLOCK,u,0u,u->record_number);
}

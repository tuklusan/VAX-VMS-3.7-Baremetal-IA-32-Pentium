#include "vms_f11_cursor.h"
/* M157: original FIL$RDWRTLBN read path; callbacks are real ATA IRPs in
 * resident SYS.EXE. Expose partial byte count on physical failure. */
uint32_t port_f11_cursor_read(struct port_f11_cursor *c,uint8_t *dst,
    uint32_t length,size_t capacity,uint32_t *completed){
    if(!c||!c->volume||!c->chain||!c->volume->read_sector||
       !dst||!completed||length>capacity||c->position>c->total_bytes)
        return PORT_F11_CURSOR_BAD;
    if(length>c->total_bytes-c->position)return PORT_F11_CURSOR_EOF;
    *completed=0u;
    while(*completed<length){
        uint32_t lbn=0u,within=0u;
        const uint32_t rc=port_f11_cursor_locate(c,&lbn,&within);
        if(rc!=PORT_F11_CURSOR_OK)return rc;
        uint8_t sector[512];
        if(c->volume->read_sector(c->volume->context,lbn,sector)!=PORT_DQ_DIR_OK)
            return PORT_F11_CURSOR_IO;
        c->reads++;
        uint32_t take=length-*completed;
        if(take>512u-within)take=512u-within;
        for(uint32_t i=0;i<take;i++)dst[*completed+i]=sector[within+i];
        c->position+=take;*completed+=take;
    }
    return PORT_F11_CURSOR_OK;
}

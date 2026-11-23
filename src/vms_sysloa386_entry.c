#include "vms_sysloa386.h"
#include <stdint.h>
/* SYSLOAxxx's native IA-32 loadable-image entry. Its self-describing
 * SYSLOAVEC data is compiled into this very image ahead of its text. */
extern const uint8_t port_sysloa386_descriptor[];
extern const uint8_t port_sysloa386_image_end[];
uint32_t port_sysloa386_mcheck_errors[2];
__attribute__((noreturn)) void port_sysloa386_unavailable(void){
    /* An unported VAX-specific vector must never succeed accidentally. */
    __asm__ volatile("cli\n1: hlt\njmp 1b");
    __builtin_unreachable();
}
uint32_t port_sysloa386_entry(const struct port_sysloa386_load_start *req){
    if(!req||req->signature!=PORT_SYSLOA386_SIGNATURE||
       req->version!=PORT_SYSLOA386_VERSION||!req->result)
        return PORT_SYSLOA386_BAD_ARGUMENT;
    const uintptr_t base=(uintptr_t)port_sysloa386_descriptor;
    const uintptr_t end=(uintptr_t)port_sysloa386_image_end;
    if(end<=base||end-base>65535u||base<0x003e0000u||end>0x003f0000u)
        return PORT_SYSLOA386_BAD_ARGUMENT;
    const uint32_t extent=(uint32_t)(end-base);
    const uint8_t *b=port_sysloa386_descriptor;
    const uint32_t n=(uint32_t)b[0]|((uint32_t)b[1]<<8)|
                     ((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);
    if(n!=extent||b[10]!=PORT_SYSLOA386_DYN_LOADCODE)return PORT_SYSLOA386_BAD_ARGUMENT;
    req->result->descriptor_address=(uint32_t)base;
    req->result->descriptor_bytes=extent;
    req->result->signature=PORT_SYSLOA386_RETURN_TAG;
    return PORT_SYSLOA386_OK;
}

#include "vms_relocdrv.h"
/* RELOCDRV.MAR EXECUTE: MOVZWL (R3)+. The decoded source stream has
 * already been range checked by port_relocdrv_decode. */
uint16_t port_relocdrv_read16(const uint8_t *p){
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8u));
}

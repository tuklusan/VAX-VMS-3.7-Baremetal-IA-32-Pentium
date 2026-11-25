#ifndef VMS_ATA_X86_H
#define VMS_ATA_X86_H
#include <stdint.h>
#include "vms_ata.h"

typedef void (*port_ata_x86_completion_fn)(void *,int);
enum { PORT_ATA_X86_ASYNC_OK=0,PORT_ATA_X86_ASYNC_BUSY=1,
       PORT_ATA_X86_ASYNC_BAD=-1 };

/* Start one native primary-master READ SECTORS command and return before
 * IRQ14. The completion callback runs from the IRQ gate and must only record
 * or enqueue completion work; it must not block or invoke user code. */
int port_ata_x86_read28_async(uint32_t,uint16_t [256],
    port_ata_x86_completion_fn,void *);
int port_ata_x86_init(void);
uint32_t port_ata_x86_irq_count(void);
void port_ata_x86_irq(void);
int port_ata_x86_read28(uint32_t,uint16_t [256]);
int port_ata_x86_write28(uint32_t,const uint16_t [256]);
#endif

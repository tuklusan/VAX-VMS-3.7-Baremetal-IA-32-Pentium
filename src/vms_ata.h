#ifndef PORTLAB_VMS_ATA_H
#define PORTLAB_VMS_ATA_H
#include <stdint.h>
/* 28-bit LBA, one ATA PIO READ SECTORS command. Read-only, no partition
 * or filesystem support. The callback bus allows deterministic host tests. */
struct port_ata_bus {
    uint8_t (*in8)(void *ctx,uint16_t port);
    uint16_t (*in16)(void *ctx,uint16_t port);
    void (*out8)(void *ctx,uint16_t port,uint8_t value);
    void *ctx;
    /* Optional paired hooks for hardware that completes a sector by IRQ. */
    int (*begin_read)(void *ctx);
    int (*wait_read)(void *ctx);
};
enum { PORT_ATA_OK=0, PORT_ATA_BAD_ARG=-1, PORT_ATA_NO_DISK=-2,
       PORT_ATA_TIMEOUT=-3, PORT_ATA_DEVICE_ERROR=-4 };
int port_ata_read28(const struct port_ata_bus *b,uint32_t lba,
                    uint16_t dst[256]);
#endif

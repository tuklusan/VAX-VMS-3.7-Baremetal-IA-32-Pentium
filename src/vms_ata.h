#ifndef PORTLAB_VMS_ATA_H
#define PORTLAB_VMS_ATA_H
#include <stdint.h>
/* 28-bit LBA, single-sector ATA PIO commands. The callback bus allows
 * deterministic host tests. Filesystem and VMS IOC policy live above it. */
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

/* Write transactions use a distinct bus contract so existing read-only
 * callers cannot accidentally acquire a write path. Native hardware polls
 * the initial DRQ phase and arms IRQ14 for command completion. */
struct port_ata_write_bus {
    uint8_t (*in8)(void *ctx,uint16_t port);
    void (*out8)(void *ctx,uint16_t port,uint8_t value);
    void (*out16)(void *ctx,uint16_t port,uint16_t value);
    void *ctx;
    int (*begin_write)(void *ctx);
    int (*wait_write_data)(void *ctx);
    int (*arm_write_complete)(void *ctx);
    int (*wait_write_complete)(void *ctx);
};
int port_ata_write28(const struct port_ata_write_bus *b,uint32_t lba,
                     const uint16_t src[256]);
#endif

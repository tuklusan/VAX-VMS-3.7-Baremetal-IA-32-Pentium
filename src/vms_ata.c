#include "vms_ata.h"
enum { ATA_DATA=0x1f0, ATA_SECCOUNT=0x1f2, ATA_LBA_LO=0x1f3,
       ATA_LBA_MID=0x1f4, ATA_LBA_HI=0x1f5,
       ATA_DRIVE=0x1f6, ATA_COMMAND=0x1f7, ATA_STATUS=0x1f7,
       ATA_ALTSTATUS=0x3f6, ATA_READ=0x20,
       ATA_BUSY=0x80, ATA_DRQ=0x08, ATA_ERROR=0x01, ATA_DF=0x20 };
/* Bounded polling: absent or broken disks cannot wedge the monitor forever. */
static int wait_status(const struct port_ata_bus *b, int require_drq) {
    for (uint32_t i=0;i<100000u;i++) {
        uint8_t st=b->in8(b->ctx,ATA_STATUS);
        if (st==0 || st==0xff) return PORT_ATA_NO_DISK;
        if (st&(ATA_ERROR|ATA_DF)) return PORT_ATA_DEVICE_ERROR;
        if (!(st&ATA_BUSY) && (!require_drq || (st&ATA_DRQ)))
            return PORT_ATA_OK;
    }
    return PORT_ATA_TIMEOUT;
}
int port_ata_read28(const struct port_ata_bus *b,uint32_t lba,
                    uint16_t dst[256]) {
    if (!b || !b->in8 || !b->in16 || !b->out8 || !dst || lba>0x0fffffffu)
        return PORT_ATA_BAD_ARG;
    /* BIOS disk enumeration may leave the *absent slave* selected.  Its
     * status reads as 0/0xff even when primary master exists.  Select the
     * desired drive BEFORE deciding whether the disk is present. */
    b->out8(b->ctx,ATA_DRIVE,(uint8_t)(0xe0u|((lba>>24)&15u)));
    /* ATA PIO: four alternate-status reads give >=400 ns select settling. */
    for (int i=0;i<4;i++) (void)b->in8(b->ctx,ATA_ALTSTATUS);
    int rc=wait_status(b,0);
    if (rc) return rc;
    b->out8(b->ctx,ATA_SECCOUNT,1);
    b->out8(b->ctx,ATA_LBA_LO,(uint8_t)lba);
    b->out8(b->ctx,ATA_LBA_MID,(uint8_t)(lba>>8));
    b->out8(b->ctx,ATA_LBA_HI,(uint8_t)(lba>>16));
    b->out8(b->ctx,ATA_COMMAND,ATA_READ);
    rc=wait_status(b,1);
    if (rc) return rc;
    for (unsigned i=0;i<256;i++) dst[i]=b->in16(b->ctx,ATA_DATA);
    return PORT_ATA_OK;
}

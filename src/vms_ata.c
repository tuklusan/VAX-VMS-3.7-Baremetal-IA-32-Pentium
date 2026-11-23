#include "vms_ata.h"
enum { ATA_DATA=0x1f0, ATA_SECCOUNT=0x1f2, ATA_LBA_LO=0x1f3,
       ATA_LBA_MID=0x1f4, ATA_LBA_HI=0x1f5,
       ATA_DRIVE=0x1f6, ATA_COMMAND=0x1f7, ATA_STATUS=0x1f7,
       ATA_ALTSTATUS=0x3f6, ATA_READ=0x20, ATA_WRITE=0x30,
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
    if (!b || !b->in8 || !b->in16 || !b->out8 || !dst || lba>0x0fffffffu ||
        (!!b->begin_read != !!b->wait_read))
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
    if(b->begin_read && b->begin_read(b->ctx))return PORT_ATA_DEVICE_ERROR;
    b->out8(b->ctx,ATA_COMMAND,ATA_READ);
    rc=b->wait_read ? b->wait_read(b->ctx) : wait_status(b,1);
    if (rc) return rc;
    for (unsigned i=0;i<256;i++) dst[i]=b->in16(b->ctx,ATA_DATA);
    return PORT_ATA_OK;
}

/* Issue one LBA28 PIO WRITE SECTORS transaction. The completion hook is
 * armed before the data words so an immediate device IRQ cannot race the
 * software arm. The non-IRQ fallback is bounded polling. */
int port_ata_write28(const struct port_ata_write_bus *b,uint32_t lba,
                     const uint16_t src[256]) {
    if(!b||!b->in8||!b->out8||!b->out16||!src||lba>0x0fffffffu)
        return PORT_ATA_BAD_ARG;
    const int hooked=!!b->begin_write||!!b->wait_write_data||
        !!b->arm_write_complete||!!b->wait_write_complete;
    if(hooked&&(!b->begin_write||!b->wait_write_data||
       !b->arm_write_complete||!b->wait_write_complete))
        return PORT_ATA_BAD_ARG;
    b->out8(b->ctx,ATA_DRIVE,(uint8_t)(0xe0u|((lba>>24)&15u)));
    for(int i=0;i<4;i++)(void)b->in8(b->ctx,ATA_ALTSTATUS);
    const struct port_ata_bus polling={.in8=b->in8,.out8=b->out8,.ctx=b->ctx};
    int rc=wait_status(&polling,0);
    if(rc)return rc;
    b->out8(b->ctx,ATA_SECCOUNT,1);
    b->out8(b->ctx,ATA_LBA_LO,(uint8_t)lba);
    b->out8(b->ctx,ATA_LBA_MID,(uint8_t)(lba>>8));
    b->out8(b->ctx,ATA_LBA_HI,(uint8_t)(lba>>16));
    if(hooked&&b->begin_write(b->ctx))return PORT_ATA_DEVICE_ERROR;
    b->out8(b->ctx,ATA_COMMAND,ATA_WRITE);
    rc=hooked?b->wait_write_data(b->ctx):wait_status(&polling,1);
    if(rc)return rc;
    if(hooked&&b->arm_write_complete(b->ctx))return PORT_ATA_DEVICE_ERROR;
    for(unsigned i=0;i<256;i++)b->out16(b->ctx,ATA_DATA,src[i]);
    if(hooked)return b->wait_write_complete(b->ctx);
    for(uint32_t i=0;i<100000u;i++){
        const uint8_t status=b->in8(b->ctx,ATA_STATUS);
        if(status==0u||status==0xffu)return PORT_ATA_NO_DISK;
        if(status&(ATA_ERROR|ATA_DF))return PORT_ATA_DEVICE_ERROR;
        if(!(status&ATA_BUSY)&&!(status&ATA_DRQ))return PORT_ATA_OK;
    }
    return PORT_ATA_TIMEOUT;
}

#ifndef VMS_DQ_IMAGEPROBE_H
#define VMS_DQ_IMAGEPROBE_H
#include "vms_dq_dir.h"
/* SYS/SRC/FILEREAD.MAR FIL$READVBN maps file header and first image
 * sector. A SYS.EXE disk probe is NOT image activation or process start. */
enum {PORT_DQ_IMAGE_OK=1u,PORT_DQ_IMAGE_BAD=2u,
      PORT_DQ_IMAGE_MAP=4u,PORT_DQ_IMAGE_HEADER=6u,
      PORT_DQ_IMAGE_FORMAT=8u};
struct port_dq_image_result {
    uint32_t header_lbn,data_lbn,entry,irp_count;
    uint32_t text_lbn,text_va,text_file_offset,text_bytes_verified;
    uint32_t text_sectors_verified;
    uint32_t text_elf_filesz;
    uint32_t aux_bytes_verified,aux_sectors_verified;
    uint32_t range_bytes_verified,range_irps;
    uint32_t path_bytes_verified,path_irps,path_entries_verified;
    uint32_t chain_bytes_verified,chain_irps,chain_headers_verified;
    uint16_t file_number,file_sequence,elf_segments,elf_type;
    uint32_t cursor_bytes_verified,cursor_irps;
    uint32_t private_ioc_reads,private_ioc_bytes;
    uint32_t f11_ioc_reads,f11_ioc_bytes;
    uint32_t f11_channel_reads,f11_channel_bytes;
};
uint32_t port_dq_image_header_lbn(const uint8_t home[512],
    const uint8_t index[512],const struct port_dq_dir_fid *,uint32_t *lbn);
uint32_t port_dq_image_verify_fh2(const uint8_t header[512],
    const struct port_dq_dir_fid *fid);
uint32_t port_dq_image_first_data(const uint8_t header[512],uint32_t *lbn);
uint32_t port_dq_image_text_lbn(const uint8_t header[512],
    const uint8_t elf[512],uint32_t *lbn,uint32_t *va,uint32_t *offset);
uint32_t port_dq_image_text_sector(const uint8_t header[512],
    const uint8_t elf[512],uint32_t sector_index,
    uint32_t *lbn,uint32_t *va,uint32_t *valid_bytes);
uint32_t port_dq_image_text_filesz(const uint8_t elf[512],
    uint32_t first_va,uint32_t file_off,uint32_t *filesz);
uint32_t port_dq_image_check_elf32(const uint8_t block[512],
    uint32_t *entry,uint16_t *segments);
#endif

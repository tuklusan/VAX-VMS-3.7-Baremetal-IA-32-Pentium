#ifndef VMS_BOOT_FILES11_H
#define VMS_BOOT_FILES11_H
#include <stdint.h>
#include <stddef.h>
#include "vms_bootio.h"

/* SHRLIB/F11DEF.B32: HM2 home block.  Reading the home block is the
 * first disk check; it does not, by itself, locate SYS.EXE. */
struct port_f11_home {
    uint32_t home_lbn;
    uint32_t alternate_home_lbn;
    uint32_t alternate_index_lbn;
    uint32_t index_bitmap_lbn;
    uint32_t max_files;
    uint16_t cluster;
    uint16_t index_bitmap_blocks;
    uint16_t reserved_files;
};
enum { PORT_F11_HOME_OK=1, PORT_F11_HOME_BADARG=0,
       PORT_F11_HOME_NOT_ODS2=2, PORT_F11_HOME_BAD_FIELDS=4,
       PORT_F11_HOME_BAD_CHECKSUM=6 };
uint32_t port_f11_home_read(const uint8_t *block, size_t bytes,
                            uint32_t expected_lbn,
                            struct port_f11_home *result);
/* SYS/SRC/FILEREAD.MAR: FIL$MOUNT loads INDEXF.SYS' first header
 * from the index bitmap area before any SYS.EXE lookup can proceed. */
struct port_f11_volume {
    struct port_f11_home home;
    uint8_t index_header[512];
    uint32_t index_header_lbn;
    uint32_t index_vbn_offset;
};
enum { PORT_F11_MOUNT_DISK_ERROR=8u, PORT_F11_MOUNT_BAD_INDEX=10u };
uint32_t port_f11_mount(const struct boo_io_driver *driver,
                        uint32_t read_function,
                        struct port_f11_volume *volume);
struct port_f11_fid {
    uint16_t number;
    uint16_t sequence;
    uint16_t volume;
};
enum { PORT_F11_FILE_EOF=12u, PORT_F11_FILE_BAD_MAP=14u,
       PORT_F11_FILE_BAD_HEADER=16u, PORT_F11_FILE_BAD_SEQ=18u };
/* FIL$STATBLK uses normalized longword (block count,LBN) pointers.
 * The SYSBOOT BOO$MAPVBN and BOO$SETMAP routines consume this list. */
struct port_f11_extent { uint32_t blocks; uint32_t lbn; };
uint32_t port_f11_collect_extents(const uint8_t header[512],
                                  struct port_f11_extent *extents,
                                  uint32_t capacity, uint32_t *count);
/* FIL$RDCHKFILHDR follows FH2$W_EXT_FID and joins all allocation
 * maps. The index header remains the root for every extension FID. */
uint32_t port_f11_collect_file_extents(const struct boo_io_driver *driver,
                                uint32_t read_function,
                                const struct port_f11_volume *volume,
                                struct port_f11_fid first,
                                struct port_f11_extent *result,
                                uint32_t capacity,uint32_t *used,
                                uint32_t *blocks,uint32_t *start_lbn);
/* FIL$READVBN and the level-2 part of FIL$RDCHKFILHDR. */
uint32_t port_f11_vbn_to_lbn(const uint8_t header[512],uint32_t vbn,
                             uint32_t *lbn);
uint32_t port_f11_read_vbn(const struct boo_io_driver *driver,
                           uint32_t read_function,const uint8_t header[512],
                           uint32_t vbn,uint8_t block[512]);
uint32_t port_f11_read_file_header(const struct boo_io_driver *driver,
                                   uint32_t read_function,
                                   const struct port_f11_volume *volume,
                                   struct port_f11_fid fid,
                                   uint8_t header[512]);
uint32_t port_f11_file_extent_stats(const uint8_t header[512],
                                   uint32_t *first_lbn,uint32_t *blocks);
/* FIL$FINDFILID: level-2 non-spanning variable-length directory records.
 * A zero version requests the first (highest-version) entry. */
enum { PORT_F11_FILE_NOT_FOUND=20u, PORT_F11_FILE_BAD_DIR=22u };
uint32_t port_f11_find_file(const struct boo_io_driver *driver,
                            uint32_t read_function,
                            const struct port_f11_volume *volume,
                            struct port_f11_fid directory,
                            const char *name_with_type,
                            uint16_t requested_version,
                            struct port_f11_fid *found);
/* FIL$OPENFILE path: MFD, directory file(s), file ID and file header.
 * Currently handles named directories and explicit decimal versions.
 * UIC directory syntax and extension header chains remain unresolved. */
uint32_t port_f11_open_named_file(const struct boo_io_driver *driver,
                                  uint32_t read_function,
                                  const struct port_f11_volume *volume,
                                  const char *filespec,
                                  struct port_f11_fid *fid,
                                  uint8_t header[512],
                                  uint32_t *first_lbn,
                                  uint32_t *blocks);
#endif

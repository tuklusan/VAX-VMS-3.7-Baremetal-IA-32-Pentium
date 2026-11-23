#ifndef VMS_SYSMOUNT_H
#define VMS_SYSMOUNT_H
#include <stdint.h>
#include <stddef.h>

/* SYSINI/SRC/SYSMOU.B32.  These are the MOUDEF parser bit numbers,
 * not flags chosen by the PC port. */
enum {
    PORT_MOUNT_OPT_SYSTEM=8, PORT_MOUNT_OPT_WRITE=9,
    PORT_MOUNT_OPT_BLOCK=15, PORT_MOUNT_OPT_OVR_ID=22,
    PORT_MOUNT_OPT_DEVICE=30, PORT_MOUNT_OPT_LABEL=31,
    PORT_MOUNT_OPT_IS_FILES11=33, PORT_MOUNT_OPT_IS_FILES11B=34,
    PORT_MOUNT_OPT_MOUNTVER=54,
    PORT_MOUNT_CLF_REBUILD=9, PORT_MOUNT_CLF_REBUILDQUO=10
};
enum {
    PORT_SYSMOUNT_FOD=1u, PORT_SYSMOUNT_AVL=2u,
    PORT_SYSMOUNT_MNT=4u, PORT_SYSMOUNT_SQD=8u,
    PORT_SYSMOUNT_NO_SERVICE=2u, PORT_SYSMOUNT_BAD_ARGUMENT=4u,
    PORT_SYSMOUNT_INCOMPLETE=6u, PORT_SYSMOUNT_UNWIND_RETURN=8u
};
/* Decoded DIB values. The provider compares all primary/secondary DIB
 * fields before declaring a device suitable for a volume. */
struct port_sysmount_device {
    char name[12];
    uint16_t unit;
    uint8_t flags;
    uint8_t reserved;
    uint32_t physical_blocks;
    uint32_t sector_bytes;
    uint8_t characteristics[32];
};
/* Global state exported to native MOUNT_DISK1/MOUNT_DISK2 and REBUILD.
 * This carries the named SYSMOU data fields across service calls. */
struct port_sysmount_descriptor {
    uint32_t length;
    const char *text;
};
struct port_sysmount_state {
    uint32_t cleanup_flags;
    uint32_t channel, mailbox_channel;
    char physical_name[24], log_buffer[20];
    uint8_t home_block[512];
    uint32_t home_block_lbn, header_lbn, device_index;
    uint32_t user_status[2], current_rvn;
    void *current_vcb, *real_rvt, *real_vcb, *real_fcb;
    void *real_wcb, *real_vca, *real_aqb;
    void *log_entry, *mtl_entry, *slog_entry, *smtl_entry;
    struct port_sysmount_device primary, secondary;
    uint64_t mount_options;
    uint32_t protection, owner_uic, user_uic;
    uint32_t extension, window, accessed, blocksize;
    uint32_t extent_cache, fid_cache, quota_cache, extent_limit;
    uint32_t device_count, label_count;
    struct port_sysmount_descriptor log_name, structure_name;
    struct port_sysmount_descriptor visual_id, comment, acp_name;
    uint32_t drive_count;
    uint8_t volume_mounted, acp_started, level2;
};
/* The actual VMS service status values come from the system service layer.
 * Do not invent the SS$_ status numbers in an unrelated module. */
struct port_sysmount_status {
    uint32_t not_file_dev, device_offline, device_mounted;
    uint32_t incorrect_label, unwind, continue_status;
};
struct port_sysmount_services {
    uint32_t (*getchn)(void *context, uint32_t channel,
                       struct port_sysmount_device *primary,
                       struct port_sysmount_device *secondary);
    uint32_t (*read_home)(void *context, uint32_t channel,
                          const char *label, struct port_sysmount_state *state);
    /* mount_disk must establish the VCB and start the real ACP.
     * Returning normal without both is not a successful system mount. */
    uint32_t (*mount_disk1)(void *context, struct port_sysmount_state *state);
    uint32_t (*mount_disk2)(void *context, struct port_sysmount_state *state);
    uint32_t (*rebuild)(void *context, uint32_t channel, uint8_t quota);
    void (*message_rebuild)(void *context);
    void (*unwind)(void *context);
    void *context;
};
/* Original MOUNT_SYSTEM and MAIN_HANDLER entry points. */
void port_sysmount_reset(struct port_sysmount_state *state);
uint32_t port_sysmount_system(struct port_sysmount_state *state,
                              const struct port_sysmount_services *services,
                              const struct port_sysmount_status *codes,
                              uint32_t system_channel);
uint32_t port_sysmount_main_handler(struct port_sysmount_state *state,
                                    const struct port_sysmount_services *services,
                                    const struct port_sysmount_status *codes,
                                    uint32_t signal_status,
                                    uint32_t *saved_r0);
#endif

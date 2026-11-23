#ifndef PORT_VMS_VALIDATE_H
#define PORT_VMS_VALIDATE_H
#include <stdint.h>
#include <stddef.h>
#include "vms_hpwd.h"

/* LOGIN/SRC/VALIDATE.MAR status values and record lookup interface. */
enum {
    PORT_UAF_NORMAL = 1,
    PORT_UAF_INVUSR = -2,
    PORT_UAF_INVPWD = -4,
    PORT_UAF_RNF = -6,
    PORT_UAF_RLK = -8,
    PORT_UAF_IO = -10,
    PORT_UAF_SNE = -12,
    PORT_UAF_SPE = -14,
    PORT_UAF_DME = -16,
    PORT_UAF_BAD_ARG = -18,
    PORT_UAF_SMALL = -20
};
enum { PORT_UAF_KEY_BYTES=12, PORT_UAF_HASH_BYTES=8,
       PORT_UAF_RECORD_BYTES=512, PORT_UAF_MAX_PASSWORD=80,
       PORT_UAF_RETRY_COUNT=2, PORT_UAF_RETRY_MS=75 };

/* A PC UAF record has a fixed username key and the original password
 * verification fields. Other record bytes pass through without alteration. */
struct port_uaf_record {
    uint8_t username[PORT_UAF_KEY_BYTES];
    uint8_t hash[PORT_UAF_HASH_BYTES];
    uint8_t algorithm;
    uint8_t salt_lo, salt_hi;
    uint8_t reserved[PORT_UAF_RECORD_BYTES-23];
};

struct port_uaf_ops {
    int (*open)(void *ctx, int shared, int system_names_only);
    int (*connect)(void *ctx);
    int (*get)(void *ctx, const uint8_t key[PORT_UAF_KEY_BYTES],
               void *record, uint16_t size);
    void (*disconnect)(void *ctx);
    void (*close)(void *ctx);
    void (*delay_ms)(void *ctx, unsigned milliseconds);
};
struct port_uaf_service {
    struct port_uaf_ops ops;
    void *context;
    uint64_t privileges;
    uint64_t allowed_privileges;
};

/* The record is delivered even for bad passwords, as in the original
 * service. Search skips password checking. A missing account gets the
 * DEFAULT record and status INVUSR, never a successful login. */
int port_uaf_searchuser(struct port_uaf_service *service,
                        const uint8_t *username, uint16_t username_len,
                        void *record, uint16_t capacity);
int port_uaf_validate(struct port_uaf_service *service,
                      const uint8_t *username, uint16_t username_len,
                      const uint8_t *password, uint16_t password_len,
                      void *record, uint16_t capacity);

/* Native PC indexed store: first sector is a sorted key/LBA directory;
 * record sectors hold one complete result each. The ATA driver supplies
 * the actual media read, while tests supply the same interface in RAM. */
typedef int (*port_uaf_read_sector)(void *ctx, uint32_t lba, uint16_t dst[256]);
struct port_uaf_disk {
    port_uaf_read_sector read;
    void *context;
    uint32_t index_lba;
    uint16_t directory[256];
    uint8_t opened;
    uint8_t connected;
    uint8_t format_version;
};
void port_uaf_disk_init(struct port_uaf_disk *disk, port_uaf_read_sector read,
                        void *context, uint32_t index_lba);
struct port_uaf_service port_uaf_disk_service(struct port_uaf_disk *disk);
uint32_t port_uaf_guest_selftest(port_uaf_read_sector read, void *context);
#endif

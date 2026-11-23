#ifndef VMS_LOGIN_FILEIO_H
#define VMS_LOGIN_FILEIO_H
#include <stdint.h>
#include <stddef.h>

/* LOGIN/SRC/FILEIO.B32: primary LOGIN streams and executive process names. */
enum {
    VMS_LGI_OK = 1, VMS_LGI_EOF = -7, VMS_LGI_IO = -1,
    VMS_LGI_ARGUMENT = -2, VMS_LGI_CLOSED = -3,
    VMS_LGI_NOSPACE = -4, VMS_LGI_SERVICE = -5
};
enum { VMS_LGI_INPUT = 1, VMS_LGI_OUTPUT = 2, VMS_LGI_LIST = 3,
       VMS_LGI_RECORD_MAX = 128, VMS_LGI_NAMEMAX = 128,
       VMS_LGI_DEVICE_MAX = 32, VMS_LGI_TIMER_ID = 99 };
enum { VMS_LGI_READ_PROMPT=1, VMS_LGI_READ_UPPER=2,
       VMS_LGI_READ_TIMEOUT=4, VMS_LGI_READ_PURGE=8, VMS_LGI_READ_QUIET=16 };

struct vms_lgi_stream {
    uint16_t ifi, isi, channel;
    uint32_t characteristics;
    uint8_t readable, concealed;
    char device[VMS_LGI_DEVICE_MAX];
    char resolved[VMS_LGI_NAMEMAX];
};
struct vms_lgi_ops {
    int (*open)(void *ctx, unsigned which, const char *spec,
                struct vms_lgi_stream *stream);
    int (*connect)(void *ctx, struct vms_lgi_stream *stream);
    int (*close)(void *ctx, struct vms_lgi_stream *stream);
    int (*flush)(void *ctx, struct vms_lgi_stream *stream);
    int (*display)(void *ctx, struct vms_lgi_stream *stream);
    int (*read)(void *ctx, struct vms_lgi_stream *stream, uint8_t *buf,
                unsigned capacity, unsigned flags, unsigned timeout_seconds,
                const char *prompt, unsigned prompt_len);
    int (*write)(void *ctx, struct vms_lgi_stream *stream,
                 const uint8_t *buf, unsigned len);
    int (*logical)(void *ctx, const char *name, const uint8_t *value,
                   unsigned length, unsigned executive_mode);
    int (*format)(void *ctx, const uint8_t *control, unsigned length,
                  const uint32_t *args, unsigned arg_count,
                  uint8_t *out, unsigned capacity);
    int (*arm_timer)(void *ctx, unsigned seconds, unsigned request_id);
    int (*cancel_timer)(void *ctx, unsigned request_id);
    int (*cancel_io)(void *ctx, uint16_t input_channel);
};
struct vms_lgi_fileio {
    struct vms_lgi_ops ops;
    void *ctx;
    struct vms_lgi_stream input, output;
    uint8_t input_open, output_open, input_shared;
    uint8_t output_filename[VMS_LGI_NAMEMAX];
    uint8_t input_dvi[VMS_LGI_DEVICE_MAX];
    uint8_t output_dvi[VMS_LGI_DEVICE_MAX];
    uint8_t input_logical[VMS_LGI_DEVICE_MAX + 5];
    uint8_t output_logical[VMS_LGI_DEVICE_MAX + 5];
    unsigned input_logical_len, output_logical_len;
    uint16_t input_channel, input_ifi, input_isi, output_ifi, output_isi;
    uint32_t input_characteristics, output_characteristics;
    const char *input_prompt;
    unsigned input_prompt_length, input_flags, input_timeout;
    uint16_t last_input_length;
    int last_status;
};
void vms_lgi_fileio_init(struct vms_lgi_fileio *io,
                         const struct vms_lgi_ops *ops, void *ctx);
int vms_lgi_open_input(struct vms_lgi_fileio *io);
int vms_lgi_close_input(struct vms_lgi_fileio *io);
int vms_lgi_open_output(struct vms_lgi_fileio *io);
int vms_lgi_close_output(struct vms_lgi_fileio *io);
int vms_lgi_write_file(struct vms_lgi_fileio *io, const char *filespec);
int vms_lgi_write_fao(struct vms_lgi_fileio *io, const uint8_t *ascic,
                      const uint32_t *args, unsigned arg_count);
int vms_lgi_write_output(struct vms_lgi_fileio *io, const uint8_t *data,
                         unsigned len);
int vms_lgi_write_timeout(struct vms_lgi_fileio *io);
int vms_lgi_get_input(struct vms_lgi_fileio *io, uint8_t *buf,
                      unsigned capacity);
#endif

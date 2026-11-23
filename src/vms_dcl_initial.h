#ifndef PORT_VMS_DCL_INITIAL_H
#define PORT_VMS_DCL_INITIAL_H
#include <stdint.h>

enum {
    PORT_DCL_INITIAL_OK=1, PORT_DCL_INITIAL_BADSTR=-1,
    PORT_DCL_INITIAL_BADCTX=-2, PORT_DCL_INITIAL_MEMORY=-3,
    PORT_DCL_INITIAL_SERVICE=-4,
    PORT_DCL_INITIAL_MAX_SYMS=72, PORT_DCL_INITIAL_MAX_VALUE=256,
    PORT_DCL_INITIAL_FRAMES=9, PORT_DCL_INITIAL_IDF_BYTES=112,
    PORT_DCL_INITIAL_RAB_LEN=68, PORT_DCL_INITIAL_FAB_LEN=80,
    PORT_DCL_INITIAL_NAM_LEN=96, PORT_DCL_INITIAL_CTX_MAX=256,
    PORT_DCL_CTX_HEADER=0,PORT_DCL_CTX_CMDSTR=1,
    PORT_DCL_CTX_LOGNAM=2,PORT_DCL_CTX_CLISYM=3,
    PORT_DCL_CTX_GLOBAL=0,PORT_DCL_CTX_LOCAL=1,
    PORT_DCL_CTX_STRING=0,PORT_DCL_CTX_PERM=1,PORT_DCL_CTX_BINARY=2,
    PORT_DCL_CTX_AUTOLOGO=1, PORT_DCL_CTX_EOFLOGO=2,
    PORT_DCL_CTX_VERIFY=4, PORT_DCL_CTRLY_MASK=(1u<<25)
};

struct port_dcl_initial_ppd {
    unsigned batch, no_control_y;
    uint16_t input_channel,input_isi,output_isi,input_ifi,output_ifi;
    uint32_t input_devchar,output_devchar;
    char input_device[16],output_device[16];
    uint8_t initial_procedures;
    uint8_t table_structure_level;
    uint32_t parent_pid;
};
struct port_dcl_initial_sym {
    char name[64];
    char value[PORT_DCL_INITIAL_MAX_VALUE+1];
    uint32_t binary;
    uint16_t length;
    uint8_t global,type,nonunique;
};
struct port_dcl_initial_rab {
    uint16_t length,isi,ifi;
    uint32_t device_characteristics;
    uint8_t mbc,mbf,prompt;
};
struct port_dcl_initial_frame {
    uint8_t content[PORT_DCL_INITIAL_IDF_BYTES];
};
struct port_dcl_initial {
    struct port_dcl_initial_ppd ppd;
    struct port_dcl_initial_sym symbols[PORT_DCL_INITIAL_MAX_SYMS];
    unsigned symbols_used;
    struct port_dcl_initial_rab input,output,alternate_input,alternate_output;
    struct port_dcl_initial_rab *active_input,*active_output,*error_stream;
    struct port_dcl_initial_rab *indirect_input,*indirect_output;
    uint16_t fab_length,nam_length;
    struct port_dcl_initial_frame frames[PORT_DCL_INITIAL_FRAMES];
    unsigned depth,frame_capacity,frame_size,frame_stack_offset;
    uint16_t flags,on_level;
    uint8_t flags2,exam_width,exam_mode,table_level,command_pending;
    unsigned verify,auto_logout,eof_logout,control_y_enabled;
    uint32_t outofband_mask,privileges[2],last_status;
    char status[11],severity[2],pending_command[512];
    unsigned command_length;
};
/* The source's GETJPI/ASSIGN/GETCHN/QIOW services enter through these
   native operations. A spawn record is accepted only from the parent PID. */
struct port_dcl_initial_ops {
    int (*translate_process)(void *,const char *,char *,unsigned);
    int (*push_procedure)(void *,const char *);
    int (*reset_oob)(void *,uint32_t);
    int (*install_change_mode)(void *);
    void (*rundown_login)(void *);
    void (*restart)(void *);
    void (*fatal)(void *,int);
    int (*open_spawn_mailbox)(void *,char *,unsigned,uint8_t *);
    int (*read_spawn_record)(void *,uint8_t *,unsigned,uint32_t *);
    void (*close_spawn_mailbox)(void *);
    void (*delete_super_error)(void *);
    int (*set_privileges)(void *,const uint32_t [2]);
    int (*create_logical)(void *,uint8_t,const char *,const char *);
};
int port_dcl_initial_start(struct port_dcl_initial *,
                           const struct port_dcl_initial_ppd *,
                           const struct port_dcl_initial_ops *,void *);
int port_dcl_initial_spawn_record(struct port_dcl_initial *,
                                  const uint8_t *,unsigned,uint32_t,
                                  const struct port_dcl_initial_ops *,void *);
int port_dcl_initial_symbol(const struct port_dcl_initial *,const char *,
                            unsigned,char *,unsigned);
void port_dcl_initial_set_status(struct port_dcl_initial *,uint32_t);
int port_dcl_initial_take_command(struct port_dcl_initial *,char *,unsigned);
const struct port_dcl_initial *port_dcl_get_prc(const struct port_dcl_initial *);
#endif

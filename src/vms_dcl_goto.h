#ifndef PORT_VMS_DCL_GOTO_H
#define PORT_VMS_DCL_GOTO_H
#include <stdint.h>

enum {PORT_DCL_GOTO_NORMAL=1,PORT_DCL_GOTO_BADLABEL=2,
      PORT_DCL_GOTO_NOMEM=4,PORT_DCL_GOTO_FINDERR=6,
      PORT_DCL_GOTO_MAX_LABEL=63,PORT_DCL_GOTO_TABLE_CAP=32};
struct port_dcl_goto_label {
    char name[PORT_DCL_GOTO_MAX_LABEL+1];
    uint8_t rfa[6];
};
struct port_dcl_goto_state {
    struct port_dcl_goto_label labels[PORT_DCL_GOTO_TABLE_CAP];
    unsigned count;
    unsigned pending;
    unsigned no_status;
    char target[PORT_DCL_GOTO_MAX_LABEL+1];
};
struct port_dcl_goto_ops {
    int (*find_rfa)(void *,const uint8_t[6]);
};
void port_dcl_goto_init(struct port_dcl_goto_state *state);
int port_dcl_position(struct port_dcl_goto_state *state,const char *name,
                      const struct port_dcl_goto_ops *ops,void *ctx);
int port_dcl_goto(struct port_dcl_goto_state *state,const char *args,
                  int random_access,const struct port_dcl_goto_ops *ops,void *ctx);
int port_dcl_goto_label_record(struct port_dcl_goto_state *state,
                               const char *label,const uint8_t rfa[6]);
void port_dcl_dealgoto(struct port_dcl_goto_state *state);
int port_dcl_goto_skipping(const struct port_dcl_goto_state *state);
#endif

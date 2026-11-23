#ifndef PORT_VMS_DCL_READREC_H
#define PORT_VMS_DCL_READREC_H

#include <stdint.h>

enum {
    PORT_DCL_RECORD_SIZE=512,
    PORT_DCL_PROMPT_SIZE=96,
    PORT_DCL_INDIRECT_LEVELS=8,
    PORT_DCL_SUBSTITUTION_LIMIT=1000,
    PORT_DCL_INPUT_OK=1,
    PORT_DCL_INPUT_EOF=0,
    PORT_DCL_INPUT_ERROR=-1,
    PORT_DCL_INPUT_RSA=-2,
    PORT_DCL_INPUT_SYMBOL=-3,
    PORT_DCL_INPUT_SPACE=-4,
    PORT_DCL_INPUT_RECURSE=-5,
    PORT_DCL_INPUT_LOGOUT=-6,
    PORT_DCL_INPUT_INDIRECT=-7,
    PORT_DCL_INPUT_BADFILE=-8
};

/* The record reader is the external RMS $GET service. A byte stream from
   COM1 and the temporary read-only IDE command-file store share this entry. */
struct port_dcl_readrec_ops {
    int (*get)(void *ctx,unsigned depth,const char *prompt,char *line,unsigned cap);
    int (*cancel)(void *ctx,unsigned depth);
    void (*disable_control_y)(void *ctx);
    void (*enable_control_y)(void *ctx);
    int (*push)(void *ctx,unsigned next_depth,const char *spec);
    void (*pop)(void *ctx,unsigned old_depth);
    int (*symbol)(void *ctx,const char *name,char *value,unsigned cap);
    int (*lexical)(void *ctx,const char *name,char *value,unsigned cap);
    void (*verify)(void *ctx,const char *line);
    void (*error)(void *ctx,int code);
    void (*set_status)(void *ctx,int code);
    void (*restart)(void *ctx);
    void (*logout)(void *ctx);
};
struct port_dcl_readrec {
    unsigned depth;
    unsigned batch;
    unsigned inquire;
    unsigned control_y_level;
    unsigned control_y_pending;
    unsigned auto_logout;
    unsigned eof_logout;
    unsigned verify;
    unsigned goto_search;
    unsigned indirect_enabled;
    unsigned trailing_space;
    unsigned last_retry_count;
    unsigned last_substitutions;
    unsigned last_was_indirect;
    int last_status;
    char prompt[PORT_DCL_PROMPT_SIZE];
    char line[PORT_DCL_RECORD_SIZE];
    unsigned length;
    unsigned char_cursor;
};

void port_dcl_readrec_init(struct port_dcl_readrec *p,int batch);
int port_dcl_input(struct port_dcl_readrec *p,
                   const struct port_dcl_readrec_ops *ops,void *ctx);
int port_dcl_expand(struct port_dcl_readrec *p,
                    const struct port_dcl_readrec_ops *ops,void *ctx);
/* DCL$SPECIAL receives a complete &symbol or @filespec token. */
int port_dcl_special(struct port_dcl_readrec *p,const char *token,
                     const struct port_dcl_readrec_ops *ops,void *ctx,
                     char *replacement,unsigned cap);
int port_dcl_charerror(struct port_dcl_readrec *p,
                       const struct port_dcl_readrec_ops *ops,void *ctx,int code);
int port_dcl_silent_logout(struct port_dcl_readrec *p,
                           const struct port_dcl_readrec_ops *ops,void *ctx);
#endif

#ifndef VMS_DCL_INQUIRE_H
#define VMS_DCL_INQUIRE_H
#include <stdint.h>

enum { PORT_DCL_INQUIRE_OK=1, PORT_DCL_INQUIRE_SYNTAX=0,
       PORT_DCL_INQUIRE_IO=-1, PORT_DCL_INQUIRE_SPACE=-2 };
enum { PORT_DCL_INQUIRE_LOCAL=0, PORT_DCL_INQUIRE_GLOBAL=1,
       PORT_DCL_INQUIRE_SYMBOL_MAX=63, PORT_DCL_INQUIRE_TEXT_MAX=127 };
struct port_dcl_inquire_request {
    char symbol[PORT_DCL_INQUIRE_SYMBOL_MAX+1];
    char prompt[PORT_DCL_INQUIRE_TEXT_MAX+1];
    unsigned global, punctuation;
};
struct port_dcl_inquire_ops {
    int (*input)(void *ctx,const char *prompt,char *answer,unsigned capacity);
    int (*store)(void *ctx,const char *symbol,const char *answer,unsigned global);
    void (*disable_indirect)(void *ctx);
    void (*disable_control_ast)(void *ctx);
};
/* DCL/SRC/INQUIRE.MAR: DCL$INQUIRE, including its 100$ option parser. */
int port_dcl_inquire_parse(const char *args,struct port_dcl_inquire_request *out);
int port_dcl_inquire(const char *args,const struct port_dcl_inquire_ops *ops,void *ctx);
#endif

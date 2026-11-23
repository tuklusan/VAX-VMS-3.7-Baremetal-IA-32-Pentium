#ifndef PORT_VMS_DCL_ON_H
#define PORT_VMS_DCL_ON_H
#include <stdint.h>

enum {
    PORT_DCL_ON_NORMAL=1, PORT_DCL_ON_BADLEVEL=2,
    PORT_DCL_ON_BADCOMMAND=4, PORT_DCL_ON_LEVEL=6,
    PORT_DCL_ON_LENGTH=8, PORT_DCL_ON_DELIMITER=10
};
enum {PORT_DCL_ON_TEXT_MAX=255, PORT_DCL_ON_DEFAULT_LEVEL=2};
struct port_dcl_on_process {
    unsigned interactive,indirect_depth;
    unsigned on_level;
    char on_error[PORT_DCL_ON_TEXT_MAX+1];
    char on_control_y[PORT_DCL_ON_TEXT_MAX+1];
    unsigned control_enabled;
};
void port_dcl_on_init(struct port_dcl_on_process *proc,unsigned interactive,
                      unsigned indirect_depth);
int port_dcl_on(const char *args,struct port_dcl_on_process *proc);
void port_dcl_on_reset(struct port_dcl_on_process *proc);
void port_dcl_on_ctly_reset(struct port_dcl_on_process *proc);
#endif

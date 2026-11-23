#ifndef VMS_LOGINCMD_H
#define VMS_LOGINCMD_H
/* LOGIN/SRC/LOGINCMD.CLD command definitions. */
enum { VMS_LOGIN_CMD_OK=1, VMS_LOGIN_CMD_PROMPT=2,
       VMS_LOGIN_CMD_SYNTAX=-1, VMS_LOGIN_CMD_TOOLONG=-2,
       VMS_LOGIN_CMD_AMBIGUOUS=-3 };
enum { VMS_LOGIN_USER_MAX=12, VMS_LOGIN_VALUE_MAX=128 };
struct vms_login_command {
    char username[VMS_LOGIN_USER_MAX+1];
    char disk[VMS_LOGIN_VALUE_MAX];
    char cli[VMS_LOGIN_VALUE_MAX];
    char command[VMS_LOGIN_VALUE_MAX];
    unsigned has_username:1;
    unsigned has_disk:1;
    unsigned has_cli:1;
    unsigned has_command_value:1;
    unsigned command_enabled:1;
};
int vms_login_parse(const char *line,struct vms_login_command *result);
#endif

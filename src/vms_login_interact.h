#ifndef VMS_LOGIN_INTERACT_H
#define VMS_LOGIN_INTERACT_H
#include <stdint.h>
#include "vms_login_fileio.h"
#include "vms_logincmd.h"
#include "vms_validate.h"

enum { VMS_LOGIN_OK=1, VMS_LOGIN_NOAUTO=0, VMS_LOGIN_AUTH=-2,
       VMS_LOGIN_FILE=-3, VMS_LOGIN_DISABLED=-4, VMS_LOGIN_CAPTIVE=-5,
       VMS_LOGIN_BADCLI=-6 };
enum { VMS_LOGIN_FLAG_DISABLED=1u,VMS_LOGIN_FLAG_CAPTIVE=2u,
       VMS_LOGIN_FLAG_NOWELCOME=4u,VMS_LOGIN_FLAG_NOMAIL=8u };
struct vms_lgi_interact_ops {
    int (*terminal_name)(void *context,char *out,unsigned capacity);
    int (*set_username)(void *context,const char *name);
    int (*set_termname)(void *context,const char *name);
    int (*system_privilege)(void *context,int enabled);
    int (*auto_user)(void *context,const char *device,char *username,unsigned cap);
    int (*purge_typeahead)(void *context);
    int (*translate)(void *context,const char *name,uint8_t *buf,unsigned cap);
    int (*local_echo)(void *context);
    int (*node_name)(void *context,char *buf,unsigned cap);
    uint32_t (*uaf_flags)(void *context,const struct port_uaf_record *uaf);
    unsigned (*mail_count)(void *context,const struct port_uaf_record *uaf);
};
struct vms_lgi_interact {
    struct vms_lgi_fileio *streams;
    struct port_uaf_service *uaf;
    struct vms_lgi_interact_ops ops;
    void *context;
    struct port_uaf_record user;
    struct vms_login_command command;
    char terminal[64];
    char username[VMS_LOGIN_USER_MAX+1];
    int user_present, auto_selected, logged_in, last_error;
};
void vms_lgi_interact_init(struct vms_lgi_interact *s,
                           struct vms_lgi_fileio *streams,
                           struct port_uaf_service *uaf,
                           const struct vms_lgi_interact_ops *ops,void *context);
int vms_lgi_init_interactive(struct vms_lgi_interact *s);
int vms_lgi_auto_login(struct vms_lgi_interact *s);
int vms_lgi_interactive_validation(struct vms_lgi_interact *s);
int vms_lgi_get_password(struct vms_lgi_interact *s);
int vms_lgi_write_announcement(struct vms_lgi_interact *s,const char *logical);
int vms_lgi_announce(struct vms_lgi_interact *s,const char *version);
void vms_lgi_zero_password(uint8_t *password,unsigned length);
#endif

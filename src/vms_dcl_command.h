#ifndef PORT_VMS_DCL_COMMAND_H
#define PORT_VMS_DCL_COMMAND_H
#include <stdint.h>

/* DCL/SRC/COMMAND.MAR: command processing and native service entries. */
enum {
    PORT_DCL_CMD_LINE=508, PORT_DCL_CMD_ARGS=16,
    PORT_DCL_CMD_PARAM_LIMIT=8, PORT_DCL_CMD_QUAL_LIMIT=24,
    PORT_DCL_CMD_PROMPT_MAX=96, PORT_DCL_CMD_ALIAS_MAX=32,
    PORT_DCL_CMD_OK=1, PORT_DCL_CMD_SKIPPED=2,
    PORT_DCL_CMD_LOGOUT=3, PORT_DCL_CMD_RESTART=4,
    PORT_DCL_CMD_SYNTAX=-1, PORT_DCL_CMD_BADVERB=-2,
    PORT_DCL_CMD_MISSING=-3, PORT_DCL_CMD_MAXPARM=-4,
    PORT_DCL_CMD_NOLIST=-5, PORT_DCL_CMD_NOCONCAT=-6,
    PORT_DCL_CMD_DELIMITER=-7, PORT_DCL_CMD_INPUT=-8,
    PORT_DCL_CMD_RECURSE=-9, PORT_DCL_CMD_MEMORY=-10,
    PORT_DCL_CMD_AST=-11,
    PORT_DCL_CMD_LIST=1u, PORT_DCL_CMD_CONCAT=2u,
    PORT_DCL_CMD_VALREQ=4u, PORT_DCL_CMD_KEYWORDS=8u,
    PORT_DCL_CMD_IMMEDIATE=1u, PORT_DCL_CMD_FOREIGN=2u,
    PORT_DCL_CMD_INTERNAL=4u, PORT_DCL_CMD_PROMPT=8u,
    PORT_DCL_CMD_RESULT_PARAMETER=1u, PORT_DCL_CMD_RESULT_COMMAND=2u,
    PORT_DCL_CMD_RESULT_QUAL=3u, PORT_DCL_CMD_RESULT_QUAL_VALUE=4u,
    PORT_DCL_CMD_RESULT_END=5u
};
struct port_dcl_command_rule {
    const char *name;
    unsigned image;
    unsigned flags;
    unsigned min_params,max_params;
    uint8_t param_flags[PORT_DCL_CMD_PARAM_LIMIT];
    const char *param_prompt[PORT_DCL_CMD_PARAM_LIMIT];
};
struct port_dcl_command_token {
    char value[PORT_DCL_CMD_LINE];
    uint16_t length;
    uint8_t type,index,separator,flags;
};
struct port_dcl_command {
    char input[PORT_DCL_CMD_LINE];
    char prompt[PORT_DCL_CMD_PROMPT_MAX];
    char verb[PORT_DCL_CMD_LINE];
    char external_image[PORT_DCL_CMD_LINE];
    char control_command[PORT_DCL_CMD_LINE];
    char chain_command[PORT_DCL_CMD_LINE];
    struct port_dcl_command_token tokens[PORT_DCL_CMD_ARGS+PORT_DCL_CMD_QUAL_LIMIT+2];
    unsigned count,param_count,qual_count,image,flags,no_status;
    unsigned depth,batch,control_y_level,control_y_pending,ast_disabled;
    unsigned goto_pending,hangup,run_default,privileged_image;
    unsigned commands,flushes,last_alias_count;
    uint32_t last_status;
    int last_error;
};
struct port_dcl_command_ops {
    const struct port_dcl_command_rule *(*find_verb)(void *,const char *);
    int (*symbol)(void *,const char *,char *,unsigned);
    int (*assign)(void *,const char *,const char *,unsigned global,unsigned numeric,
                  unsigned substring);
    int (*label)(void *,const char *);
    int (*seek_label)(void *,const char *);
    int (*open_procedure)(void *,const char *);
    int (*read_parameter)(void *,const char *,char *,unsigned);
    int (*dispatch)(void *,const struct port_dcl_command *,
                    const struct port_dcl_command_rule *);
    void (*flush)(void *);
    void (*control_ast)(void *,unsigned enabled);
    int (*force_exit)(void *);
    void (*report)(void *,int);
    void (*set_status)(void *,uint32_t);
};
void port_dcl_command_init(struct port_dcl_command *);
void port_dcl_cmd_start(struct port_dcl_command *,unsigned depth,
                        unsigned control_y_level);
int port_dcl_command_run(struct port_dcl_command *,const char *,
                         const struct port_dcl_command_ops *,void *);
int port_dcl_cmd_abort(struct port_dcl_command *,
                       const struct port_dcl_command_ops *,void *);
int port_dcl_cmd_eod(struct port_dcl_command *,const char *);
int port_dcl_cmd_check_ast(struct port_dcl_command *);
void port_dcl_cmd_disable(struct port_dcl_command *,
                          const struct port_dcl_command_ops *,void *);
void port_dcl_cmd_enable(struct port_dcl_command *,
                         const struct port_dcl_command_ops *,void *);
void port_dcl_cmd_flush(struct port_dcl_command *,unsigned terminal,
                        const struct port_dcl_command_ops *,void *);
const struct port_dcl_command_token *port_dcl_cmd_parameter(
    const struct port_dcl_command *,unsigned index);
const struct port_dcl_command_token *port_dcl_cmd_option(
    const struct port_dcl_command *,const char *name);
#endif

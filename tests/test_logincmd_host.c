#include "vms_logincmd.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
    struct vms_login_command c;
    assert(vms_login_parse("LOGIN",&c)==VMS_LOGIN_CMD_PROMPT && c.command_enabled);
    assert(vms_login_parse("LOGIN SYSTEM",&c)==VMS_LOGIN_CMD_OK &&
           strcmp(c.username,"SYSTEM")==0 && c.command_enabled);
    assert(vms_login_parse("login system/disk=dua0:/cli=dcl/command=boot.com",&c)==VMS_LOGIN_CMD_OK &&
           strcmp(c.username,"SYSTEM")==0 && strcmp(c.disk,"DUA0:")==0 &&
           strcmp(c.cli,"DCL")==0 && strcmp(c.command,"BOOT.COM")==0 &&
           c.has_disk && c.has_cli && c.has_command_value);
    assert(vms_login_parse("LOGIN /CLI=\"SYS$SYSTEM:DCL.EXE\" SYSTEM",&c)==VMS_LOGIN_CMD_OK &&
           strcmp(c.cli,"SYS$SYSTEM:DCL.EXE")==0);
    assert(vms_login_parse("LOGIN /DISK=DKA0:",&c)==VMS_LOGIN_CMD_PROMPT);
    assert(vms_login_parse("LOGIN SYSTEM/NOCOMMAND",&c)==VMS_LOGIN_CMD_OK && !c.command_enabled);
    assert(vms_login_parse("LOGIN SYSTEM/COMMAND",&c)==VMS_LOGIN_CMD_OK &&
           c.command_enabled && !c.has_command_value);
    assert(vms_login_parse("LOGIN SYSTEM/C=FOO",&c)==VMS_LOGIN_CMD_AMBIGUOUS);
    assert(vms_login_parse("LOGIN SYSTEM/DISK",&c)==VMS_LOGIN_CMD_SYNTAX);
    assert(vms_login_parse("LOGIN SYSTEM/CLI",&c)==VMS_LOGIN_CMD_SYNTAX);
    assert(vms_login_parse("LOGIN SYSTEM/UNKNOWN",&c)==VMS_LOGIN_CMD_SYNTAX);
    assert(vms_login_parse("LOGIN SYSTEM SYSTEM",&c)==VMS_LOGIN_CMD_SYNTAX);
    assert(vms_login_parse("LOGIN EXTRAORDINARYUSER",&c)==VMS_LOGIN_CMD_TOOLONG);
    assert(vms_login_parse("",&c)==VMS_LOGIN_CMD_SYNTAX);
    assert(vms_login_parse("SET HOST 0",&c)==VMS_LOGIN_CMD_SYNTAX);
    puts("LOGIN command username prompt, disk/CLI/command, quoted text and errors PASS");
    return 0;
}

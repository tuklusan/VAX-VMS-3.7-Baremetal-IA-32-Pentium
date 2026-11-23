#include "vms_dcl_intimages.h"
static const char *const names[PORT_DCL_IMAGE_COUNT]={
    "ALLOCATE",
    "ASSIGN",
    "CLOSE",
    "CONTINUE",
    "DEALLOCAT",
    "DEASSIGN",
    "DEBUG",
    "DECK",
    "DEFINE",
    "DEPOSIT",
    "DELSYM",
    "EOD",
    "EXAMINE",
    "EXIT",
    "EXTIMAGE",
    "GOTO",
    "IF",
    "INQUIRE",
    "LOGOUT",
    "MCR",
    "ON",
    "OPEN",
    "READ",
    "RUN",
    "SETCTLY",
    "SETDEFALT",
    "SETON",
    "SETPROT",
    "SETUIC",
    "SETVERIFY",
    "SHOWDEF",
    "SHOWPROT",
    "SHOWQUOTA",
    "SHOWSTAT",
    "SHOWSYMBL",
    "SHOWTIME",
    "SHOWTRAN",
    "STOP",
    "WAIT",
    "WRITE",
    "SPAWN",
    "ATTACH",
    "EOJ",
};
static int upper(int c){return c>='a'&&c<='z'?c-('a'-'A'):c;}
static int same(const char *a,const char *b){
    while(*a&&*b){if(upper(*a++)!=*b++)return 0;}
    return !*a&&!*b;
}
unsigned port_dcl_image_lookup(const char *name){
    if(!name)return 0;
    for(unsigned i=0;i<PORT_DCL_IMAGE_COUNT;i++)
        if(same(name,names[i]))return 0x80u+i;
    return 0;
}
const char *port_dcl_image_name(unsigned code){
    return code>=0x80u&&code<0x80u+PORT_DCL_IMAGE_COUNT?
           names[code-0x80u]:0;
}

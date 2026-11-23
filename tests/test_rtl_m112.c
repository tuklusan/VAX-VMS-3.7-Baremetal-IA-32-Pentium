#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_for_m112.h"
static unsigned errors=0;
#define CHECK(x) do { if(!(x)){fprintf(stderr,"M112 test %s:%d: %s\n",__FILE__,__LINE__,#x);errors++;} }while(0)
struct mock {unsigned calls[10];int deny;enum port112_action last_action;uint32_t last_unit,last_type,last_record;};
static int op(void *ctx,enum port112_action act,uint32_t unit,uint32_t type,uint32_t record){
 struct mock *m=(struct mock *)ctx;
 if(m->deny)return PORT112_IO_ERROR;
 if(act<1||act>9)return PORT112_IO_ERROR;
 m->calls[(unsigned)act]++;
 m->last_action=act;m->last_unit=unit;m->last_type=type;m->last_record=record;
 return PORT112_OK;
}
struct testcase{char const *name;int (*fn)(struct port112_unit *,const struct port112_backend *);uint32_t flags;};
static const struct testcase cases[]={
 {"FORDECOMF",port112_fordecomf,12u },
 {"FORDECOMO",port112_fordecomo,12u+PORT112_OBJ_FMT },
 {"FORENCOMF",port112_forencomf,11u },
 {"FORENCOMO",port112_forencomo,11u+PORT112_OBJ_FMT },
 {"FORREADDF",port112_forreaddf,6u },
 {"FORREADDO",port112_forreaddo,6u+PORT112_OBJ_FMT },
 {"FORREADDU",port112_forreaddu,8u },
 {"FORREADIF",port112_forreadif,18u },
 {"FORREADIO",port112_forreadio,18u+PORT112_OBJ_FMT },
 {"FORREADKF",port112_forreadkf,14u },
 {"FORREADKO",port112_forreadko,14u+PORT112_OBJ_FMT },
 {"FORREADKU",port112_forreadku,16u },
 {"FORREADSF",port112_forreadsf,2u },
 {"FORREADSL",port112_forreadsl,10u },
 {"FORREADSN",port112_forreadsn,20u },
 {"FORREADSO",port112_forreadso,2u+PORT112_OBJ_FMT },
 {"FORREADSU",port112_forreadsu,4u },
 {"FORREWRSF",port112_forrewrsf,13u },
 {"FORREWRSO",port112_forrewrso,13u+PORT112_OBJ_FMT },
 {"FORREWRSU",port112_forrewrsu,15u },
 {"FORWRITDF",port112_forwritdf,5u },
 {"FORWRITDO",port112_forwritdo,5u+PORT112_OBJ_FMT },
 {"FORWRITDU",port112_forwritdu,7u },
 {"FORWRITIF",port112_forwritif,17u },
 {"FORWRITIO",port112_forwritio,17u+PORT112_OBJ_FMT },
 {"FORWRITSF",port112_forwritsf,1u },
 {"FORWRITSL",port112_forwritsl,9u },
 {"FORWRITSN",port112_forwritsn,19u },
 {"FORWRITSO",port112_forwritso,1u+PORT112_OBJ_FMT },
 {"FORWRITSU",port112_forwritsu,3u },
};
static void dispatch_tests(void){
 for(unsigned i=0;i<sizeof cases/sizeof cases[0];i++){
  struct port112_unit u; struct mock m={{0},0,0,0,0,0};
  struct port112_backend b={&m,op};
  CHECK(port112_unit_init(&u,25u)==PORT112_OK);
  CHECK(cases[i].fn(&u,&b)==PORT112_OK);
  CHECK(u.in_statement==1u);
  CHECK(u.statement_kind==cases[i].flags);
  CHECK(u.current_args==(port112_statement_argument_flags(cases[i].flags&255u)|
          ((cases[i].flags&PORT112_OBJ_FMT)?0x40u:0u)));
  CHECK(u.err_arg_index==port112_statement_error_arg_index(cases[i].flags&255u));
  CHECK(m.calls[PORT112_ACT_BEGIN]==1u);
  CHECK(m.last_type==cases[i].flags && m.last_unit==25u);
  unsigned internal=(cases[i].flags&255u)==11u||(cases[i].flags&255u)==12u||
    (cases[i].flags&255u)==17u||(cases[i].flags&255u)==18u;
  CHECK(m.calls[PORT112_ACT_OPEN]==(internal?0u:1u));
  CHECK(cases[i].fn(&u,&b)==PORT112_BUSY);
  CHECK(port112_forioend(&u,&b)==PORT112_OK);
  CHECK(m.calls[PORT112_ACT_END]==1u);
  CHECK(u.in_statement==0u);
  CHECK(port112_forioend(&u,&b)==PORT112_STATE);
 }
 CHECK(sizeof cases/sizeof cases[0]==30u);
}
static void unit_tests(void){
 struct port112_unit u;struct mock m={{0},0,0,0,0,0};struct port112_backend b={&m,op};
 CHECK(port112_unit_init(NULL,1)==PORT112_BADARG);
 CHECK(port112_unit_init(&u,100000)==PORT112_BADARG);
 CHECK(port112_unit_init(&u,5)==PORT112_OK);
 CHECK(port112_foriobeg(&u,&b,0)==PORT112_BADARG);
 CHECK(port112_foriobeg(&u,&b,21)==PORT112_BADARG);
 CHECK(port112_foriobeg(&u,&b,PORT112_RSU+PORT112_OBJ_FMT)==PORT112_MODE);
 CHECK(!u.opened && !u.in_statement);
 CHECK(port112_forreadsf(&u,NULL)==PORT112_NO_BACKEND);
 CHECK(!u.opened);
 m.deny=1;
 CHECK(port112_forreadsf(&u,&b)==PORT112_IO_ERROR);
 CHECK(!u.opened && !u.in_statement);
 m.deny=0;
 CHECK(port112_forreadsf(&u,&b)==PORT112_OK);
 CHECK(u.opened && u.in_statement);
 CHECK(u.attributes==(PORT112_SEQUENTIAL|PORT112_FORMATTED));
 CHECK(port112_forclose(&u,&b)==PORT112_BUSY);
 m.deny=1;
 CHECK(port112_forioend(&u,&b)==PORT112_IO_ERROR);
 CHECK(u.in_statement);
 m.deny=0;
 CHECK(port112_forioend(&u,&b)==PORT112_OK);
 u.attributes|=PORT112_READ_ONLY;
 CHECK(port112_forwritsf(&u,&b)==PORT112_MODE);
 CHECK(port112_forreadsf(&u,&b)==PORT112_OK);
 CHECK(port112_forioend(&u,&b)==PORT112_OK);
 u.record_number=15u;
 uint32_t assoc=0;
 u.associated_variable=&assoc;u.associated_bytes=4u;
 CHECK(port112_forassoc(&u)==PORT112_OK && assoc==15u);
 u.attributes|=PORT112_APPEND;
 CHECK(port112_forbacksp(&u,&b)==PORT112_MODE);
 u.attributes&=~(PORT112_APPEND|PORT112_READ_ONLY);
 m.deny=1;CHECK(port112_forbacksp(&u,&b)==PORT112_IO_ERROR && u.record_number==15u);
 m.deny=0;CHECK(port112_forbacksp(&u,&b)==PORT112_OK && u.record_number==14u && assoc==14u);
 u.attributes|=PORT112_APPEND;
 CHECK(port112_forrewind(&u,&b)==PORT112_OK && u.record_number==1u && assoc==1u);
 CHECK((u.attributes&PORT112_APPEND)==0u);
 CHECK(port112_forendfil(&u,&b)==PORT112_OK);
 CHECK(port112_forunlock(&u,&b)==PORT112_MODE);
 CHECK(port112_forclose(&u,&b)==PORT112_OK && !u.opened);
 CHECK(port112_forclose(&u,&b)==PORT112_OK);
 CHECK(port112_forrewind(&u,&b)==PORT112_OK);
 CHECK(port112_forendfil(&u,&b)==PORT112_OK);
}
static void define_tests(void){
 struct port112_unit u;struct mock m={{0},0,0,0,0,0};struct port112_backend b={&m,op};
 uint32_t assoc=999u;
 CHECK(port112_unit_init(&u,8)==PORT112_OK);
 CHECK(port112_fordefine_l(&u,200,32768,&assoc)==PORT112_RANGE);
 CHECK(port112_fordefine_l(&u,200,10,&assoc)==PORT112_OK);
 CHECK(u.record_bytes==20u && u.maximum_records==200u);
 CHECK(u.attributes==(PORT112_DIRECT|PORT112_UNFORMATTED));
 CHECK(port112_forwritdu(&u,&b)==PORT112_OK);
 CHECK(u.attributes==(PORT112_DIRECT|PORT112_UNFORMATTED));
 CHECK(m.calls[PORT112_ACT_OPEN]==1u);
 CHECK(port112_forioend(&u,&b)==PORT112_OK);
 CHECK(port112_fordelete(&u,&b)==PORT112_OK);
 CHECK(port112_fordelete_d(&u,&b)==PORT112_OK);
 CHECK(port112_forbacksp(&u,&b)==PORT112_MODE);
 CHECK(port112_forrewind(&u,&b)==PORT112_MODE);
 CHECK(port112_forendfil(&u,&b)==PORT112_MODE);
 CHECK(port112_forclose(&u,&b)==PORT112_OK);
 CHECK(port112_unit_init(&u,8)==PORT112_OK);
 uint16_t short_assoc=0;
 CHECK(port112_fordefine_w(&u,99,5,&short_assoc)==PORT112_OK);
 u.record_number=0x123456u;
 CHECK(port112_forassoc(&u)==PORT112_OK && short_assoc==0x3456u);
 CHECK(port112_fordefine_w(&u,100,5,&short_assoc)==PORT112_STATE);
 CHECK(port112_unit_init(&u,8)==PORT112_OK);
 CHECK(port112_define(&u,20,2,&assoc,1)==PORT112_RANGE);
 CHECK(port112_define(&u,20,2,NULL,2)==PORT112_RANGE);
 CHECK(port112_define(&u,0,2,&assoc,4)==PORT112_RANGE);
}
int main(void){
 dispatch_tests();unit_tests();define_tests();
 for(unsigned i=1;i<=20;i++){
    CHECK(port112_statement_error_arg_index(i)>=2u);
    CHECK(port112_statement_error_arg_index(i)<=6u);
 }
 CHECK(port112_statement_error_arg_index(21)==0u);
 if(errors)return 1;
 puts("M112: 40 original FORTRAN RTL files, 30 exact statement selectors and 10 native partial I/O controls passed");
 return 0;
}

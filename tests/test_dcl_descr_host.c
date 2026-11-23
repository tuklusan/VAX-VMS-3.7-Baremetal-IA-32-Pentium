#include <assert.h>
#include "vms_dcl_descr.h"
#define PTR(v,o,f,t,y) ((unsigned)(v)|((unsigned)(o)<<8)|((unsigned)(f)<<20)|((unsigned)(t)<<24)|((unsigned)(y)<<28))
int main(void){
  const uint32_t input[]={PTR(7,5,2,3,4),PTR(9,0,0,2,1)};
  struct port_dcl_descr_state s={input,input+2,"SHOW ONE",8u};
  struct port_dcl_descr_result r;
  assert(port_dcl_getdval(&s,&r)==1&&r.value==7u&&
         *r.item=='O'&&r.flags==2u&&r.term==3u&&r.type==4u);
  assert(port_dcl_getdval(&s,&r)==1&&r.value==9u&&*r.item=='S');
  assert(port_dcl_getdval(&s,&r)==0);
  const uint32_t invalid[]={PTR(1,15,0,0,0)};
  s.next=invalid;s.end=invalid+1;
  assert(port_dcl_getdval(&s,&r)==0&&s.next==invalid);
  return 0;
}

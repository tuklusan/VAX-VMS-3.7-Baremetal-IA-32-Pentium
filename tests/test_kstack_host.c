#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "vms_kstack.h"
static struct port_kernel_stack pair[2];
int main(void) {
    for (unsigned i=0;i<2;i++) {
        struct port_kernel_stack *s=&pair[i];
        assert(port_kstack_guard(s)%4096u==0);
        assert(port_kstack_base(s)==port_kstack_guard(s)+4096u);
        assert(port_kstack_top(s)==port_kstack_base(s)+4096u);
        port_kstack_init(s);
        assert(port_kstack_intact(s));
        assert(port_kstack_contains_frame(s,port_kstack_top(s)-60u,60u));
        assert(!port_kstack_contains_frame(s,port_kstack_top(s)-59u,60u));
        assert(!port_kstack_contains_frame(s,port_kstack_base(s),60u));
        assert(!port_kstack_contains_frame(s,port_kstack_guard(s),60u));
        assert(!port_kstack_contains_frame(s,UINTPTR_MAX-7u,60u));
        assert(!port_kstack_contains_frame(s,port_kstack_top(s),0));
        s->data[0]=0;
        assert(!port_kstack_intact(s));
        port_kstack_init(s);
        assert(port_kstack_intact(s));
    }
    assert(port_kstack_top(&pair[0])<=port_kstack_guard(&pair[1]));
    puts("Kernel stack PASS: alignment, guard layout, bounds, canary");
}

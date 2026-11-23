#include "vms_kstack.h"
void port_kstack_init(struct port_kernel_stack *stack) {
    if (stack) stack->data[0]=PORT_KSTACK_CANARY;
}
uintptr_t port_kstack_base(const struct port_kernel_stack *stack) {
    return stack ? (uintptr_t)&stack->data[0] : 0;
}
uintptr_t port_kstack_top(const struct port_kernel_stack *stack) {
    return stack ? (uintptr_t)&stack->data[PORT_KSTACK_WORDS] : 0;
}
uintptr_t port_kstack_guard(const struct port_kernel_stack *stack) {
    return stack ? (uintptr_t)&stack->guard[0] : 0;
}
int port_kstack_contains_frame(const struct port_kernel_stack *stack,
                               uintptr_t frame, size_t size) {
    if (!stack || size==0) return 0;
    uintptr_t base=port_kstack_base(stack)+sizeof(uint32_t);
    uintptr_t top=port_kstack_top(stack);
    return frame>=base && frame<=top && size<=top-frame;
}
int port_kstack_intact(const struct port_kernel_stack *stack) {
    return stack && stack->data[0]==PORT_KSTACK_CANARY;
}

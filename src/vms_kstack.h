#ifndef PORTLAB_VMS_KSTACK_H
#define PORTLAB_VMS_KSTACK_H
#include <stdint.h>
#include <stddef.h>

/* Each CPL3 task has one supervisor-only interrupt stack. A non-present
 * page immediately below it catches downward stack overflows. IA-32 trap
 * stubs execute on the active task's stack selected by TSS.esp0. */
enum { PORT_KSTACK_PAGE_BYTES=4096u, PORT_KSTACK_WORDS=1024u };
#define PORT_KSTACK_CANARY 0x37C0A11Du
struct port_kernel_stack {
    uint8_t guard[PORT_KSTACK_PAGE_BYTES];
    uint32_t data[PORT_KSTACK_WORDS];
} __attribute__((aligned(PORT_KSTACK_PAGE_BYTES)));
_Static_assert(sizeof(struct port_kernel_stack)==8192u,"stack/guard layout");
void port_kstack_init(struct port_kernel_stack *stack);
uintptr_t port_kstack_base(const struct port_kernel_stack *stack);
uintptr_t port_kstack_top(const struct port_kernel_stack *stack);
uintptr_t port_kstack_guard(const struct port_kernel_stack *stack);
int port_kstack_contains_frame(const struct port_kernel_stack *stack,
                               uintptr_t frame, size_t size);
int port_kstack_intact(const struct port_kernel_stack *stack);
#endif

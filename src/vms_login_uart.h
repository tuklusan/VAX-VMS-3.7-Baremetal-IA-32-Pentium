#ifndef VMS_LOGIN_UART_H
#define VMS_LOGIN_UART_H
#include "vms_login_fileio.h"
struct vms_lgi_uart {
    uint8_t logical[4][VMS_LGI_DEVICE_MAX+5];
    unsigned length[4];
    uint32_t input_count, output_count, canceled;
    uint32_t (*ticks)(void);
    unsigned arm_at, timer_id, timer_active;
};
void vms_lgi_uart_init(struct vms_lgi_uart *io, uint32_t (*ticks)(void));
const struct vms_lgi_ops *vms_lgi_uart_ops(void);
#endif

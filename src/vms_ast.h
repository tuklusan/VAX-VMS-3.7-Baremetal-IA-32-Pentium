#ifndef PORTLAB_VMS_AST_H
#define PORTLAB_VMS_AST_H
#include <stdint.h>

/* Architecture-neutral AST contract for the IA-32 port experiment.
 * All operations require caller serialization. No VAX ABI is implied. */
enum { VMS_MODE_KERNEL=0, VMS_MODE_EXEC=1, VMS_MODE_SUPER=2,
       VMS_MODE_USER=3, VMS_AST_NONE_LEVEL=4, VMS_AST_QUOTA=16 };
enum vms_ast_kind { VMS_AST_NORMAL=0, VMS_AST_SPECIAL_KERNEL=1 };
enum vms_ast_result {
    VMS_AST_OK=0, VMS_AST_EMPTY=1, VMS_AST_DEFERRED=2,
    VMS_AST_BAD_ARGUMENT=-1, VMS_AST_BAD_PID=-2,
    VMS_AST_QUOTA_EXCEEDED=-3, VMS_AST_ALREADY_QUEUED=-4
};
typedef void (*vms_ast_callback)(void *context, uint32_t parameter);
struct vms_ast_request {
    struct vms_ast_request *next;
    uint32_t pid;
    uint32_t parameter;
    vms_ast_callback callback;
    void *context;
    uint8_t mode;
    uint8_t kind;
    uint8_t state; /* 0 free, 1 queued, 2 in callback */
};
struct vms_ast_process {
    uint32_t pid;
    struct vms_ast_request *queue;
    uint16_t queued;
    uint16_t quota_used;
    uint32_t delivered;
    uint8_t enable_mask; /* bit per VMS access mode */
    uint8_t active_mask;
    uint8_t software_ipl; /* 0..31; AST gate at IPL 2 */
    uint8_t ast_level;    /* 0..3 or 4 for empty */
};
void vms_ast_init(struct vms_ast_process *p, uint32_t pid);
int vms_ast_set_enable(struct vms_ast_process *p, unsigned mode, int enabled);
int vms_ast_set_ipl(struct vms_ast_process *p, unsigned ipl);
int vms_ast_enqueue(struct vms_ast_process *p, struct vms_ast_request *r);
int vms_ast_take(struct vms_ast_process *p, unsigned returning_mode,
                 struct vms_ast_request **picked);
int vms_ast_complete(struct vms_ast_process *p, struct vms_ast_request *r);
int vms_ast_dispatch_one(struct vms_ast_process *p, unsigned returning_mode);
int vms_ast_cancel(struct vms_ast_process *p, struct vms_ast_request *r);
uint8_t vms_ast_compute_level(const struct vms_ast_process *p);
#endif

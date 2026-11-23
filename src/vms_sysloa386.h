#ifndef VMS_SYSLOA386_H
#define VMS_SYSLOA386_H
#include <stdint.h>
/* Native IA-32 CPU-dependent SYSLOA386.EXE, corresponding to the
 * SYSLOAxxx/LOAVEC processor-dependent image. VAX bus machinery is not
 * represented by fake PC devices. Unimplemented vectors fail closed. */
enum {
    PORT_SYSLOA386_SIGNATURE=0x36384c53u,
    PORT_SYSLOA386_VERSION=1u,
    PORT_SYSLOA386_RETURN_TAG=0x4b4e4c53u,
    PORT_SYSLOA386_DYN_LOADCODE=98u,
    PORT_SYSLOA386_OK=1u,
    PORT_SYSLOA386_BAD_ARGUMENT=2u,
    PORT_SYSLOA386_BAD_CPU=4u,
    PORT_SYSLOA386_BAD_DISK=6u
};
struct port_sysloa386_load_result {
    uint32_t signature;
    uint32_t descriptor_address;
    uint32_t descriptor_bytes;
};
struct port_sysloa386_load_start {
    uint32_t signature,version;
    struct port_sysloa386_load_result *result;
};
/* Snapshot is recorded from SIDT, MOV CR0/CR3, STR and live IDT gates.
 * `ticks` comes from the installed PIT IRQ handler, not a timer simulation. */
struct port_sysloa386_gate {
    uint16_t offset_low,selector;
    uint8_t reserved,type;
    uint16_t offset_high;
} __attribute__((packed));
struct port_sysloa386_cpu_result {
    uint32_t cr0,cr3,idt_base,irq_ticks;
    uint16_t idt_limit,task_selector,kernel_cs,reserved;
    struct port_sysloa386_gate gp,pf,timer;
};
struct port_sysloa386_cpu_ops {
    void *context;
    uint32_t (*read_irq_ticks)(void *);
};
struct port_sysloa386_boot_result {
    uint32_t home_block_lbn;
    uint32_t physical_boot_sector_checksum;
    uint16_t cluster_factor,system_structure_level;
};
struct port_sysloa386_boot_ops {
    void *context;
    int (*read_sector)(void *,uint32_t,uint16_t [256]);
};
uint32_t port_sysloa386_entry(const struct port_sysloa386_load_start *req);
uint32_t port_sysloa386_cpu_probe(const struct port_sysloa386_cpu_ops *ops,
                                   struct port_sysloa386_cpu_result *out);
uint32_t port_sysloa386_boot_probe(const struct port_sysloa386_boot_ops *ops,
                                    struct port_sysloa386_boot_result *out);
uint32_t port_sysloa386_check_cpu(const struct port_sysloa386_cpu_result *regs);
uint32_t port_sysloa386_check_home(const uint8_t sector[512],
                                   struct port_sysloa386_boot_result *out);
#endif

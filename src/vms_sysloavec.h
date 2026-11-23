#ifndef VMS_SYSLOAVEC_H
#define VMS_SYSLOAVEC_H
#include <stdint.h>
#include <stddef.h>

/* SYS/SRC/SYSLOAVEC.MAR, resident and PRMSW loadable-image forms. */
enum port_sysloa_vector {
    PORT_SYSLOA_MCHK,
    PORT_SYSLOA_INT54,
    PORT_SYSLOA_INT58,
    PORT_SYSLOA_INT5C,
    PORT_SYSLOA_INT60,
    PORT_SYSLOA_UBA_INT0,
    PORT_SYSLOA_UBA_UNEXINT,
    PORT_SYSLOA_ECC_REENABLE,
    PORT_SYSLOA_INIBOOTADP,
    PORT_SYSLOA_DUMPCPUREG,
    PORT_SYSLOA_REGRESTOR,
    PORT_SYSLOA_REGSAVE,
    PORT_SYSLOA_INIPROCREG,
    PORT_SYSLOA_TEST_CSR,
    PORT_SYSLOA_PURGDATAP,
    PORT_SYSLOA_DW780_INT,
    PORT_SYSLOA_RH780_INT,
    PORT_SYSLOA_CI_INITIAL,
    PORT_SYSLOA_CI_INT,
    PORT_SYSLOA_UBA_INITIAL,
    PORT_SYSLOA_INI_MPMADP,
    PORT_SYSLOA_SHUTDWNADP,
    PORT_SYSLOA_MCHK_ERRCNT,
    PORT_SYSLOA_COUNT
};

enum { PORT_SYSLOA_HEADER_BYTES=12u, PORT_SYSLOA_PATCH_WORDS=15u };
struct port_sysloa_table {
    uintptr_t address[PORT_SYSLOA_COUNT];
    uint8_t loaded;
};
struct port_sysloa_patch {
    uint32_t count;
    uint32_t words[PORT_SYSLOA_PATCH_WORDS];
};

void port_sysloavec_init(struct port_sysloa_table *resident);
int port_sysloavec_link(struct port_sysloa_table *resident,
                        const void *loaded_image, uint32_t bytes,
                        uint8_t code_type);
uintptr_t port_sysloavec_address(const struct port_sysloa_table *resident,
                                 unsigned vector);
/* An unresolved executable entry follows EXE$LOAD_ERROR's halt path. */
__attribute__((noreturn)) void port_sysloavec_unloaded(void);
void port_sysloavec_patch_init(struct port_sysloa_patch *patch);
#endif

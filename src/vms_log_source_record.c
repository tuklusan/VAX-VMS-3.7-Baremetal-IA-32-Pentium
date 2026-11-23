#include "vms_log_system.h"
/* Never dereference untrusted list pointers; map only the two source
 * records published after actual SYS.EXE FIN_DATABASE validation. */
uint8_t *port_log_source_record(const struct port_log_system_table *t,
                                uint32_t address){
    if(!t||!address)return 0;
    for(unsigned i=0u;i<2u;i++)
        if(t->record_addr[i]==address)return t->records[i];
    return 0;
}

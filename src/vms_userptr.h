#ifndef PORTLAB_USERPTR_H
#define PORTLAB_USERPTR_H
#include <stdint.h>
#define PORT_USER_RANGE_MAX 8192u
#define PORT_USER_OK 0
#define PORT_USER_BAD (-1)
/* The virtual memory map is kernel-owned. Resolver maps trusted physical
 * page-table addresses to kernel-accessible PTE arrays (host testable). */
typedef const uint32_t *(*port_pt_resolver)(void *context,uint32_t table_phys);
/* Resolve a trusted physical page to its kernel-accessible linear alias. */
typedef uint8_t *(*port_phys_page_resolver)(void *context,uint32_t physical_page);
struct port_user_map {
    const uint32_t *pde;
    port_pt_resolver resolve;
    void *context;
};
int port_user_range_valid(const struct port_user_map *map,
                          uint32_t address,uint32_t size,int writing);
/* QIO transfer probes preserve the full VMS longword byte count. This walks
 * every IA-32 page without imposing the smaller synchronous-copy bound. */
int port_user_io_range_valid(const struct port_user_map *map,
                             uint32_t address,uint32_t size,int writing);
/* Pin pages intersecting a VMS QIO transfer. The retained set supports
 * 17 pages; larger requests fail closed. Release it once after completion. */
#define PORT_USER_PIN_MAX_PAGES 17u
typedef int (*port_user_page_pin)(void *context,uint32_t physical_page);
typedef void (*port_user_page_unpin)(void *context,uint32_t physical_page);
struct port_user_page_locks {
    uint32_t physical[PORT_USER_PIN_MAX_PAGES];
    unsigned count;
    uint32_t virtual_base,transfer_base,transfer_bytes;
    port_phys_page_resolver physical_resolver;
    void *physical_context;
    port_user_page_unpin release;
    void *release_context;
};
int port_user_pin_io_range(const struct port_user_map *map,uint32_t address,
                           uint32_t size,int writing,
                           port_phys_page_resolver physical,void *physical_context,
                           port_user_page_pin pin,port_user_page_unpin unpin,
                           void *lock_context,struct port_user_page_locks *locks);
void port_user_unpin_io_range(struct port_user_page_locks *locks);
/* Async I/O completion/source staging through the pinned physical aliases;
 * the current CR3 and original user VA are not consulted for translation. */
int port_user_copy_to_pinned(const struct port_user_page_locks *,uint32_t,
                             const void *,uint32_t);
int port_user_copy_from_pinned(const struct port_user_page_locks *,void *,
                               uint32_t,uint32_t);
/* Validate all user PTEs and all destination physical spans before writing
 * any byte. Works when user virtual pages are not identity-mapped. */
int port_user_copy_to(const struct port_user_map *map, uint32_t dest,
                      const void *source, uint32_t bytes,
                      port_phys_page_resolver physical, void *context);
/* Copy from a mapped CPL3 source to a trusted kernel destination.  The
 * output buffer remains untouched if any virtual/physical page is absent. */
int port_user_copy_from(const struct port_user_map *map, void *dest,
                        uint32_t source, uint32_t bytes,
                        port_phys_page_resolver physical, void *context);
/* Commit up to two separate user output buffers as a single transaction.
 * All mappings, physical aliases and output disjointness are checked before
 * either user byte changes. Used for block DATA+IOSB completion. */
#define PORT_USER_OUTPUT_MAX 2u
struct port_user_output {
    uint32_t destination;
    const void *source;
    uint32_t bytes;
};
/* Check that two output virtual spans are writable, physically backed,
 * and disjoint even across aliases. No bytes are written. */
int port_user_outputs_disjoint(const struct port_user_map *map,
                               uint32_t data_va,uint32_t data_bytes,
                               uint32_t status_va,uint32_t status_bytes,
                               port_phys_page_resolver physical,void *context);
int port_user_copyv_to(const struct port_user_map *map,
                       const struct port_user_output *outputs,
                       unsigned count, port_phys_page_resolver physical,
                       void *context);
#endif

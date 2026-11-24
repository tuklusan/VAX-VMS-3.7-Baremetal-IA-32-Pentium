#ifndef VMS_EXE_IO_H
#define VMS_EXE_IO_H
#include <stdint.h>
#include "vms_exe_gettim.h"
#include "vms_exe_access.h"
/* Native CCB, IRP, UCB and VCB fields needed by the original I/O services. */
enum {
 PORT_EXE_DEV_ALLOCATED=1u<<0,PORT_EXE_DEV_MOUNTED=1u<<1,
 PORT_EXE_DEV_FOREIGN=1u<<2,PORT_EXE_DEV_SPOOLED=1u<<3,
 PORT_EXE_DEV_SHAREABLE=1u<<4,PORT_EXE_DEV_MAILBOX=1u<<5,
 PORT_EXE_DEV_TERMINAL=1u<<6,PORT_EXE_DEV_MARK_DISMOUNT=1u<<7,
 PORT_EXE_DEV_OPERATOR=1u<<8,PORT_EXE_DEV_VALID=1u<<9,
 PORT_EXE_DEV_SOFT_LOCK=1u<<10,PORT_EXE_DEV_RCK=1u<<11,
 PORT_EXE_DEV_WCK=1u<<12
};
enum {
 PORT_SS_CANCEL=2096u, PORT_SS_FILNOTACC=172u,
 PORT_SS_DEVASSIGN=2120u,PORT_SS_DEVMOUNT=108u,
 PORT_SS_DEVNOTALLOC=2136u,PORT_SS_DEVALLOC=2112u,
 PORT_SS_IVDEVNAM=324u,PORT_SS_IVLOGNAM=340u,PORT_SS_NOIOCHAN=436u,
 PORT_SS_NOSUCHDEV=2312u,
 PORT_SS_DEVALRALLOC=1601u,PORT_SS_BUFFEROVF=1537u
};
enum {PORT_EXE_CANCEL_NORMAL=0u,PORT_EXE_CANCEL_DEASSIGN=1u};
struct port_exe_device {
    const char *name;
    uint32_t flags,owner_pid,owner_uic;
    uint16_t references,volume_protection;
    uint8_t alloc_mode,driver_ipl;
    void *volume, *driver_context;
    struct port_exe_device *mailbox;
};
struct port_exe_io_request {
    struct port_exe_io_request *next;
    uint32_t pid;
    uint16_t channel,status;
    uint8_t virtual_io,buffered,queued;
};
struct port_exe_io_channel {
    uint32_t owner_pid;
    uint16_t channel,outstanding;
    uint8_t assigned,access_mode,has_file,associated_mailbox;
    struct port_exe_device *device;
    struct port_exe_io_request *requests;
};
struct port_exe_io_ops {
    void *context;
    uint32_t (*verify_channel)(void *,uint32_t,uint16_t,struct port_exe_io_channel **);
    uint32_t (*driver_cancel)(void *,struct port_exe_device *,uint16_t,uint8_t);
    uint32_t (*post_io)(void *,struct port_exe_io_request *);
    uint32_t (*deaccess)(void *,struct port_exe_io_channel *);
    uint32_t (*wait_io)(void *,struct port_exe_io_channel *);
    uint32_t (*release_volume)(void *,void *);
    uint32_t (*delete_mailbox)(void *,struct port_exe_device *);
    void (*lock)(void *);
    void (*unlock)(void *);
};
#endif
